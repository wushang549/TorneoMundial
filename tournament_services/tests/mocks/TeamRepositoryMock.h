#pragma once
#include <gmock/gmock.h>
#include <string_view>
#include <memory>
#include <vector>
#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"

using TeamRepoIface = IRepository<domain::Team, std::string_view>;

class MockTeamRepository : public TeamRepoIface {
public:
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view), (override));
};
