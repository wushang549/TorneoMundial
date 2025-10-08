#pragma once
#include <gmock/gmock.h>
#include <string_view>
#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"

// OJO: si IRepository vive en un namespace, ajusta:
// using TeamRepoIface = persistence::repository::IRepository<domain::Team, std::string_view>;
using TeamRepoIface = IRepository<domain::Team, std::string_view>;

class MockTeamRepository : public TeamRepoIface {
public:
    // Create retorna Id (string_view)
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));

    // ReadAll retorna vector de shared_ptr<Type>
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));

    // ReadById recibe Id = string_view
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));

    // Update retorna Id = string_view
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));

    // Delete recibe Id = string_view
    MOCK_METHOD(void, Delete, (std::string_view), (override));
};
