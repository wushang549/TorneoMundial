// MatchDelegate.hpp
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <expected>
#include <nlohmann/json.hpp>
#include "delegate/IMatchDelegate.hpp"    // <-- use the single source of truth
#include "domain/Match.hpp"
#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"


class IMatchRepository;
class ITournamentDelegate;

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;

    static std::string pickDeterministicWinner(const std::string& tournamentId,
                                               const std::string& matchId,
                                               const std::string& homeTeamId,
                                               const std::string& visitorTeamId);
public:
    MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                  std::shared_ptr<ITournamentDelegate> tournamentDel);

    std::vector<std::shared_ptr<domain::Match>>
    ReadAll(const std::string& tournamentId,
            const std::optional<std::string_view>& showFilter) override;

    std::shared_ptr<domain::Match>
    ReadById(const std::string& tournamentId, const std::string& matchId) override;

    std::expected<void, std::string>
    UpdateScore(const std::string& tournamentId,
                const std::string& matchId,
                int homeScore, int visitorScore) override;

    // NEW
    std::expected<std::string, std::string>
    Create(const std::string& tournamentId, const nlohmann::json& body) override;
    virtual void ProcessTeamAddition(const TeamAddEvent& evt);
    virtual void ProcessScoreUpdate(const ScoreUpdateEvent& evt);
};
