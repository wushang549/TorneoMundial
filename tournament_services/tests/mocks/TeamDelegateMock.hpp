#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string_view>
#include <vector>

#include "delegate/ITeamDelegate.hpp"   // asegúrate de que existe
#include "domain/Team.hpp"

class TeamDelegateMock : public ITeamDelegate {
public:

    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (std::string_view), (override));
    MOCK_METHOD(bool, UpdateTeam, (std::string_view, const domain::Team&), (override));
    MOCK_METHOD(bool, DeleteTeam, (std::string_view), (override));
};
