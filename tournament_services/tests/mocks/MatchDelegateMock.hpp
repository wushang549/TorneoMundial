#pragma once
#include <gmock/gmock.h>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

#include "delegate/MatchDelegate.hpp"
#include "domain/Match.hpp"

class MatchDelegateMock : public MatchDelegate {
public:
    // Base ctor expects two shared_ptrs; for tests we can pass nullptr.
    MatchDelegateMock()
        : MatchDelegate(nullptr, nullptr) {}

    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>),
                ReadAll,
                (const std::string& tournamentId,
                 const std::optional<std::string_view>& filter),
                (override));

    MOCK_METHOD(std::shared_ptr<domain::Match>,
                ReadById,
                (const std::string& tournamentId, const std::string& matchId),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                UpdateScore,
                (const std::string& tournamentId, const std::string& matchId,
                 int home, int visitor),
                (override));

    MOCK_METHOD((std::expected<std::string, std::string>),
                Create,
                (const std::string& tournamentId, const nlohmann::json& body),
                (override));

    MOCK_METHOD(void,
                ProcessTeamAddition,
                (const TeamAddEvent& evt),
                (override));

    // New method for ScoreUpdateListener tests
    MOCK_METHOD(void,
                  ProcessScoreUpdate,
                  (const ScoreUpdateEvent& evt),
                  (override));

};
