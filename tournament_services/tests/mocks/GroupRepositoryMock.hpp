#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "persistence/repository/IGroupRepository.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

class GroupRepositoryMock : public IGroupRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Group&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));

    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId,
                (const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId,
                (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId,
                (const std::string_view&, const std::string_view&), (override));

    MOCK_METHOD(void, UpdateGroupAddTeam,
                (const std::string_view&, const std::shared_ptr<domain::Team>&), (override));
};
