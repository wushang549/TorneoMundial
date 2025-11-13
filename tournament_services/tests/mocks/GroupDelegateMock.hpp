#pragma once

#include <gmock/gmock.h>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"

class GroupDelegateMock : public IGroupDelegate
{
public:
    MOCK_METHOD((std::expected<std::string, std::string>),
                CreateGroup,
                (std::string_view tournamentId, const domain::Group& group),
                (override));

    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>),
                GetGroups,
                (std::string_view tournamentId),
                (override));

    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>),
                GetGroup,
                (std::string_view tournamentId, std::string_view groupId),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                UpdateGroup,
                (std::string_view tournamentId, const domain::Group& group),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                RemoveGroup,
                (std::string_view tournamentId, std::string_view groupId),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                UpdateTeams,
                (std::string_view tournamentId,
                 std::string_view groupId,
                 const std::vector<domain::Team>& teams),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                AddTeamToGroup,
                (std::string_view tournamentId,
                 std::string_view groupId,
                 std::string_view teamId),
                (override));
};
