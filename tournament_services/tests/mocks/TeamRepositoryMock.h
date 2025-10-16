#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include "persistence/repository/TeamRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "domain/Team.hpp"

class TeamRepositoryMock : public TeamRepository {
public:
    TeamRepositoryMock()
    : TeamRepository(std::shared_ptr<IDbConnectionProvider>{}) {}

    // Firmas EXACTAS del repo real (usa std::string_view)
    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view), (override));
};

// ✅ Alias para compatibilidad con el nombre viejo
using MockTeamRepository = TeamRepositoryMock;
