#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string_view>
#include "delegate/ITeamDelegate.hpp"
#include "domain/Team.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team& team), (override));
    MOCK_METHOD(bool, UpdateTeam, (std::string_view id, const domain::Team& team), (override));
    MOCK_METHOD(bool, DeleteTeam, (std::string_view id), (override));
};
