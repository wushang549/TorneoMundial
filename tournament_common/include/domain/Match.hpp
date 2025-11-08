//Match.hpp
#pragma once
#include <string>
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

namespace domain {

/**
 * Lightweight team reference stored inside a Match.
 * API shape: { "id": "...", "name": "..." }
 */
class TeamRef {
    std::string id_;
    std::string name_;
public:
    TeamRef() = default;
    TeamRef(std::string id, std::string name)
        : id_(std::move(id)), name_(std::move(name)) {}

    const std::string& Id() const { return id_; }
    const std::string& Name() const { return name_; }

    std::string& Id() { return id_; }
    std::string& Name() { return name_; }
};

/**
 * Domain model for a match.
 * Supports: listing, scoring, winner resolution, and knockout progression linkage.
 */
class Match {
    std::string id_;
    std::string tournamentId_;
    std::string round_;          // "group", "r16", "qf", "sf", "final" (free string for flexibility)
    TeamRef home_;
    TeamRef visitor_;
    std::optional<int> scoreHome_;
    std::optional<int> scoreVisitor_;
    std::string status_ = "pending";        // "pending" | "played"
    std::optional<std::string> winnerTeamId_;
    std::optional<std::string> decidedBy_;  // "regularTime" | "randomTieBreak"

    // Knockout progression pointers
    std::optional<std::string> nextMatchId_;          // UUID of the next match in the bracket
    std::optional<std::string> nextMatchWinnerSlot_;  // "home" | "visitor"

public:
    Match() = default;

    // Next match linkage (used for knockout progression)
    const std::optional<std::string>& NextMatchId() const { return nextMatchId_; }
    void SetNextMatchId(const std::string& id) { nextMatchId_ = id; }

    const std::optional<std::string>& NextMatchWinnerSlot() const { return nextMatchWinnerSlot_; }
    void SetNextMatchWinnerSlot(const std::string& slot) { nextMatchWinnerSlot_ = slot; }

    // Basic getters (const + non-const where useful)
    const std::string& Id() const { return id_; }
    std::string& Id() { return id_; }

    const std::string& TournamentId() const { return tournamentId_; }
    std::string& TournamentId() { return tournamentId_; }

    const std::string& Round() const { return round_; }
    std::string& Round() { return round_; }

    const TeamRef& Home() const { return home_; }
    TeamRef& Home() { return home_; }

    const TeamRef& Visitor() const { return visitor_; }
    TeamRef& Visitor() { return visitor_; }

    const std::string& Status() const { return status_; }
    std::string& Status() { return status_; }

    const std::optional<std::string>& WinnerTeamId() const { return winnerTeamId_; }
    const std::optional<std::string>& DecidedBy() const { return decidedBy_; }

    // Mutators used by the delegate when applying results
    void SetScore(int home, int visitor) {
        scoreHome_ = home;
        scoreVisitor_ = visitor;
    }
    void ClearScore() {
        scoreHome_.reset();
        scoreVisitor_.reset();
    }

    void SetWinnerTeamId(const std::string& id) { winnerTeamId_ = id; }
    void SetDecidedBy(const std::string& how)   { decidedBy_ = how; }
    void SetStatus(const std::string& s)        { status_ = s; }

    // Convenience accessors for score presence/value
    bool HasScore() const { return scoreHome_.has_value() && scoreVisitor_.has_value(); }
    std::optional<int> ScoreHome() const { return scoreHome_; }
    std::optional<int> ScoreVisitor() const { return scoreVisitor_; }

    // JSON mappers (API shape)
    friend void to_json(nlohmann::json& j, const Match& m);
    friend void from_json(const nlohmann::json& j, Match& m);
};

// ---------- JSON for TeamRef ----------
inline void to_json(nlohmann::json& j, const TeamRef& t) {
    j = nlohmann::json{
        {"id",   t.Id()},
        {"name", t.Name()}
    };
}

inline void from_json(const nlohmann::json& j, TeamRef& t) {
    t.Id()   = j.value("id", "");
    t.Name() = j.value("name", "");
}

// ---------- JSON for Match ----------
inline void to_json(nlohmann::json& j, const Match& m) {
    j = nlohmann::json{
        {"id",           m.Id()},
        {"tournamentId", m.TournamentId()},
        {"round",        m.Round()},
        {"home",         m.Home()},
        {"visitor",      m.Visitor()},
        {"status",       m.Status()}
    };

    // Only emit "score" when both values are present (avoid partial score shape)
    if (m.HasScore()) {
        const auto homeVal    = static_cast<nlohmann::json::number_integer_t>(*m.ScoreHome());
        const auto visitorVal = static_cast<nlohmann::json::number_integer_t>(*m.ScoreVisitor());

        nlohmann::json score = nlohmann::json::object();
        score["home"]    = homeVal;
        score["visitor"] = visitorVal;

        j["score"] = std::move(score);
    }


    // Optional winner/decision
    if (m.winnerTeamId_.has_value()) j["winnerTeamId"] = *m.winnerTeamId_;
    if (m.decidedBy_.has_value())    j["decidedBy"]    = *m.decidedBy_;

    // Optional knockout linkage
    if (m.nextMatchId_.has_value())         j["nextMatchId"]         = *m.nextMatchId_;
    if (m.nextMatchWinnerSlot_.has_value()) j["nextMatchWinnerSlot"] = *m.nextMatchWinnerSlot_;
}

inline void from_json(const nlohmann::json& j, Match& m) {
    m.Id()           = j.value("id", "");
    m.TournamentId() = j.value("tournamentId", "");
    m.Round()        = j.value("round", "");
    m.Status()       = j.value("status", "pending");

    // Assign TeamRef fields explicitly (avoids clangd/operator= issues)
    if (j.contains("home") && j["home"].is_object()) {
        const auto& h = j["home"];
        m.Home().Id()   = h.value("id", "");
        m.Home().Name() = h.value("name", "");
    }
    if (j.contains("visitor") && j["visitor"].is_object()) {
        const auto& v = j["visitor"];
        m.Visitor().Id()   = v.value("id", "");
        m.Visitor().Name() = v.value("name", "");
    }

    // Optional score
    if (j.contains("score") && j["score"].is_object()) {
        const auto& s = j["score"];
        if (s.contains("home") && s.contains("visitor") &&
            s["home"].is_number_integer() && s["visitor"].is_number_integer()) {
            m.SetScore(s["home"].get<int>(), s["visitor"].get<int>());
        } else {
            m.ClearScore();
        }
    } else {
        m.ClearScore();
    }

    // Optional winner/decision
    if (j.contains("winnerTeamId") && j["winnerTeamId"].is_string())
        m.winnerTeamId_ = j["winnerTeamId"].get<std::string>();
    if (j.contains("decidedBy") && j["decidedBy"].is_string())
        m.decidedBy_    = j["decidedBy"].get<std::string>();

    // Optional knockout linkage
    if (j.contains("nextMatchId") && j["nextMatchId"].is_string())
        m.nextMatchId_ = j["nextMatchId"].get<std::string>();
    if (j.contains("nextMatchWinnerSlot") && j["nextMatchWinnerSlot"].is_string())
        m.nextMatchWinnerSlot_ = j["nextMatchWinnerSlot"].get<std::string>();
}

} // namespace domain
