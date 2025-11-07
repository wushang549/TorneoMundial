#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace domain {

/**
 * Lightweight team reference stored inside a Match.
 * Mirrors the API shape: { "id": "...", "name": "..." }
 */
class TeamRef {
    std::string id_;
    std::string name_;
public:
    TeamRef() = default;
    TeamRef(std::string id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

    const std::string& Id() const { return id_; }
    const std::string& Name() const { return name_; }

    std::string& Id() { return id_; }
    std::string& Name() { return name_; }
};

/**
 * Domain model for a match.
 * Minimal fields to support: listing, scoring, winner resolution and knockout progression.
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

public:
    Match() = default;

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

    // Only emit "score" when both values are present (API expects it omitted for pending)
    // inside: inline void from_json(const nlohmann::json& j, Match& m)
    class TeamRef {
    public:
        TeamRef& operator=(const TeamRef&) = default;
        TeamRef& operator=(TeamRef&&) noexcept = default;
    };


    // Optional winner/decision
    if (m.winnerTeamId_.has_value()) j["winnerTeamId"] = *m.winnerTeamId_;
    if (m.decidedBy_.has_value())    j["decidedBy"]    = *m.decidedBy_;
}

inline void from_json(const nlohmann::json& j, Match& m) {
    m.Id()           = j.value("id", "");
    m.TournamentId() = j.value("tournamentId", "");
    m.Round()        = j.value("round", "");
    m.Status()       = j.value("status", "pending");

    if (j.contains("home"))    m.Home()    = j.at("home").get<TeamRef>();
    if (j.contains("visitor")) m.Visitor() = j.at("visitor").get<TeamRef>();

    if (j.contains("score") && j["score"].is_object()) {
        const auto& s = j["score"];
        if (s.contains("home") && s.contains("visitor") &&
            s["home"].is_number_integer() && s["visitor"].is_number_integer()) {
            m.SetScore(s["home"].get<int>(), s["visitor"].get<int>());
        }
    } else {
        m.ClearScore();
    }

    if (j.contains("winnerTeamId")) m.winnerTeamId_ = j["winnerTeamId"].get<std::string>();
    if (j.contains("decidedBy"))    m.decidedBy_    = j["decidedBy"].get<std::string>();
}

} // namespace domain
