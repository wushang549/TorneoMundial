#pragma once

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Team.hpp"

class IGroupDelegate {
public:
    virtual ~IGroupDelegate() = default;

    // Create a group under a tournament
    virtual std::expected<std::string, std::string>
    CreateGroup(std::string_view tournamentId, const domain::Group& group) = 0;

    // Read groups of a tournament
    virtual std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
    GetGroups(std::string_view tournamentId) = 0;

    // Read a specific group by (tournament, group)
    virtual std::expected<std::shared_ptr<domain::Group>, std::string>
    GetGroup(std::string_view tournamentId, std::string_view groupId) = 0;

    // Update/remove group (if you need later)
    virtual std::expected<void, std::string>
    UpdateGroup(std::string_view tournamentId, const domain::Group& group) = 0;

    virtual std::expected<void, std::string>
    RemoveGroup(std::string_view tournamentId, std::string_view groupId) = 0;

    // Replace/append teams to a given group (batch)
    virtual std::expected<void, std::string>
    UpdateTeams(std::string_view tournamentId, std::string_view groupId,
                const std::vector<domain::Team>& teams) = 0;

    // Add one team to a specific group (idempotent if already in the tournament)
    virtual std::expected<void, std::string>
    AddTeamToGroup(std::string_view tournamentId,
                   std::string_view groupId,
                   std::string_view teamId) = 0;
};
