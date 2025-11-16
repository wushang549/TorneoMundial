#include "delegate/MatchDelegate.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <regex>
#include <stdexcept>

#include "persistence/repository/IMatchRepository.hpp"
#include "delegate/ITournamentDelegate.hpp"

using std::string;
using std::string_view;

// Score must be in [0, 10]
static inline bool is_valid_score(int s) noexcept { return s >= 0 && s <= 10; }

namespace {
const std::regex UUID_RE_DELEG(
    R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)"
);
}

MatchDelegate::MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                             std::shared_ptr<ITournamentDelegate> tournamentDel)
    : matchRepository(std::move(matchRepo)),
      tournamentDelegate(std::move(tournamentDel)) {}

// ---------- helpers ----------

std::string MatchDelegate::pickDeterministicWinner(const std::string& tournamentId,
                                                   const std::string& matchId,
                                                   const std::string& homeTeamId,
                                                   const std::string& visitorTeamId) {
    // Deterministic "random" winner based on IDs
    const std::string key = tournamentId + '|' + matchId;
    const size_t seed = std::hash<std::string>{}(key);
    std::mt19937 gen(static_cast<uint32_t>(seed));
    std::uniform_int_distribution<int> dist(0, 1);
    return (dist(gen) == 0) ? homeTeamId : visitorTeamId;
}

// ---------- ReadAll / ReadById ----------

std::vector<std::shared_ptr<domain::Match>>
MatchDelegate::ReadAll(const std::string& tournamentId,
                       const std::optional<std::string_view>& showFilter) {
    if (!tournamentDelegate) {
        throw std::runtime_error("not_found");
    }

    auto t = tournamentDelegate->ReadById(tournamentId);
    if (!t.has_value()) {
        // Controllers map this runtime_error to HTTP 404.
        throw std::runtime_error("not_found");
    }

    auto matches = matchRepository->FindByTournamentId(tournamentId);

    if (showFilter.has_value()) {
        const auto f = *showFilter;
        if (f == "played" || f == "pending") {
            const bool wantPlayed = (f == "played");
            matches.erase(
                std::remove_if(matches.begin(), matches.end(),
                               [wantPlayed](const std::shared_ptr<domain::Match>& m) {
                                   const bool isPlayed = (m->Status() == "played");
                                   return isPlayed != wantPlayed;
                               }),
                matches.end());
        }
    }

    return matches;
}

std::shared_ptr<domain::Match>
MatchDelegate::ReadById(const std::string& tournamentId,
                        const std::string& matchId) {
    return matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
}

// ---------- UpdateScore ----------

std::expected<void, std::string>
MatchDelegate::UpdateScore(const std::string& tournamentId,
                           const std::string& matchId,
                           int homeScore, int visitorScore) {
    // Early validation: repo must not be touched on invalid scores.
    if (!is_valid_score(homeScore) || !is_valid_score(visitorScore)) {
        return std::unexpected("validation:score_out_of_range");
    }

    auto m = matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
    if (!m) {
        return std::unexpected("not_found");
    }

    // Set score inside domain entity.
    m->SetScore(homeScore, visitorScore);

    std::string winnerId;
    std::string decidedBy = "regularTime";

    if (homeScore > visitorScore) {
        winnerId = m->Home().Id();
    } else if (visitorScore > homeScore) {
        winnerId = m->Visitor().Id();
    } else {
        decidedBy = "randomTieBreak";
        winnerId = pickDeterministicWinner(
            tournamentId, matchId, m->Home().Id(), m->Visitor().Id());
    }

    m->SetWinnerTeamId(winnerId);
    m->SetDecidedBy(decidedBy);
    m->SetStatus("played");

    try {
        matchRepository->Update(*m);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("unexpected:") + e.what());
    }
}

// ---------- Create ----------

std::expected<std::string, std::string>
MatchDelegate::Create(const std::string& tournamentId,
                      const nlohmann::json& body) {
    if (!tournamentDelegate) {
        return std::unexpected("not_found");
    }

    auto t = tournamentDelegate->ReadById(tournamentId);
    if (!t.has_value()) {
        return std::unexpected("not_found");
    }

    // round
    if (!body.contains("round") || !body["round"].is_string()) {
        return std::unexpected("validation:missing_or_invalid_round");
    }

    // home
    if (!body.contains("home") || !body["home"].is_object() ||
        !body["home"].contains("id") || !body["home"]["id"].is_string() ||
        !body["home"].contains("name") || !body["home"]["name"].is_string()) {
        return std::unexpected("validation:missing_or_invalid_home");
    }

    // visitor
    if (!body.contains("visitor") || !body["visitor"].is_object() ||
        !body["visitor"].contains("id") || !body["visitor"]["id"].is_string() ||
        !body["visitor"].contains("name") || !body["visitor"]["name"].is_string()) {
        return std::unexpected("validation:missing_or_invalid_visitor");
    }

    const std::string round       = body["round"].get<std::string>();
    const std::string homeId      = body["home"]["id"].get<std::string>();
    const std::string homeName    = body["home"]["name"].get<std::string>();
    const std::string visitorId   = body["visitor"]["id"].get<std::string>();
    const std::string visitorName = body["visitor"]["name"].get<std::string>();

    if (round.empty()) {
        return std::unexpected("validation:round_empty");
    }
    if (!std::regex_match(homeId, UUID_RE_DELEG) ||
        !std::regex_match(visitorId, UUID_RE_DELEG)) {
        return std::unexpected("validation:team_id_not_uuid");
    }
    if (homeId == visitorId) {
        return std::unexpected("validation:same_team_ids");
    }

    domain::Match m;
    m.TournamentId() = tournamentId;
    m.Round()        = round;
    m.Home().Id()    = homeId;
    m.Home().Name()  = homeName;
    m.Visitor().Id() = visitorId;
    m.Visitor().Name() = visitorName;
    m.Status()       = "pending";

    try {
        const std::string id = matchRepository->Create(m);
        return id;
    } catch (const std::exception& e) {
        return std::unexpected(std::string("unexpected:") + e.what());
    }
}

// ---------- ProcessTeamAddition ----------

void MatchDelegate::ProcessTeamAddition(const TeamAddEvent& evt) {
    // Intentionally no-op in tournament_services.
    (void)evt;
}
void MatchDelegate::ProcessScoreUpdate(const ScoreUpdateEvent& evt) {
    // In tournament_services we do not handle this event directly.
    // It is consumed in tournament_consumer.
    (void)evt;
}