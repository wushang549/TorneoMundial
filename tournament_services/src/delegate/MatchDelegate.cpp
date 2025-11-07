#include "delegate/MatchDelegate.hpp"
#include <functional>
#include <random>
#include <stdexcept>
#include <algorithm>

#include "persistence/repository/IMatchRepository.hpp"
#include "delegate/ITournamentDelegate.hpp"

using std::string;
using std::string_view;

// Simple score validator for the 0..10 rule used across the project.
static inline bool is_valid_score(int s) noexcept { return s >= 0 && s <= 10; }

MatchDelegate::MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                             std::shared_ptr<ITournamentDelegate> tournamentDel)
    : matchRepository(std::move(matchRepo)),
      tournamentDelegate(std::move(tournamentDel)) {}

std::vector<std::shared_ptr<domain::Match>>
MatchDelegate::ReadAll(const std::string& tournamentId,
                       const std::optional<std::string_view>& showFilter) {
    // Per spec: return 404 if the tournament does not exist.
    // We do this check at delegate level to keep controllers thin.
    if (!tournamentDelegate || !tournamentDelegate->ReadById(tournamentId)) {
        // Controller will map runtime_error("not_found") -> 404
        throw std::runtime_error("not_found");
    }

    // Fetch from repository (no filter assumed at repo level to keep interface simple).
    auto matches = matchRepository->FindByTournamentId(tournamentId);

    // Apply in-memory filter if requested. If you later add a repo-side filter,
    // this remains correct and cheap for small result sets.
    if (showFilter.has_value()) {
        const auto f = *showFilter;
        if (f == "played" || f == "pending") {
            const bool wantPlayed = (f == "played");
            matches.erase(
                std::remove_if(matches.begin(), matches.end(),
                    [&](const std::shared_ptr<domain::Match>& m) {
                        return (m->Status() == "played") != wantPlayed;
                    }),
                matches.end()
            );
        }
    }
    return matches;
}

std::shared_ptr<domain::Match>
MatchDelegate::ReadById(const std::string& tournamentId, const std::string& matchId) {
    // We don't hard-fail on missing tournament here; reading by id is enough.
    // Controller will map nullptr to 404.
    return matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
}

std::string MatchDelegate::pickDeterministicWinner(const std::string& tournamentId,
                                                   const std::string& matchId,
                                                   const std::string& homeTeamId,
                                                   const std::string& visitorTeamId) {
    // Deterministic random: seed derived from immutable IDs.
    // That makes retries idempotent and tests reproducible.
    const std::string key = tournamentId + '|' + matchId;
    const size_t seed = std::hash<std::string>{}(key);
    std::mt19937 gen(static_cast<uint32_t>(seed));
    std::uniform_int_distribution<int> dist(0, 1);
    return (dist(gen) == 0) ? homeTeamId : visitorTeamId;
}

std::expected<void, std::string>
MatchDelegate::UpdateScore(const std::string& tournamentId,
                           const std::string& matchId,
                           int homeScore, int visitorScore) {
    // Validate score limits upfront so the repository isn't called with bad data.
    if (!is_valid_score(homeScore) || !is_valid_score(visitorScore)) {
        return std::unexpected("validation:score_out_of_range");
    }

    // Load the match instance.
    auto m = matchRepository->FindByTournamentIdAndMatchId(tournamentId, matchId);
    if (!m) {
        return std::unexpected("not_found");
    }

    // Apply the provided score.
    // Expect domain::Match to expose SetScore(int,int)
    // and accessors Home()/Visitor() with .Id().
    m->SetScore(homeScore, visitorScore);

    // Decide winner: the project forbids draws across all phases.
    std::string winnerId;
    std::string decidedBy = "regularTime";
    if (homeScore > visitorScore) {
        winnerId = m->Home().Id();
    } else if (visitorScore > homeScore) {
        winnerId = m->Visitor().Id();
    } else {
        // Equal score -> deterministic tiebreak so we never bounce on retries.
        decidedBy = "randomTieBreak";
        winnerId = pickDeterministicWinner(tournamentId, matchId, m->Home().Id(), m->Visitor().Id());
    }

    // Persist winner, decision source, and final status.
    m->SetWinnerTeamId(winnerId);
    m->SetDecidedBy(decidedBy);
    m->SetStatus("played");

    try {
        // Repository should perform the actual UPDATE and return the id or void.
        matchRepository->Update(*m);
        return {};
    } catch (const std::exception& e) {
        // Bubble up a tagged error so the controller maps to 500.
        return std::unexpected(std::string("unexpected:") + e.what());
    }
}
