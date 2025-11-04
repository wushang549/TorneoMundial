#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "domain/Tournament.hpp"

class TournamentRepositoryMock : public TournamentRepository {
public:
    TournamentRepositoryMock()
    : TournamentRepository(std::shared_ptr<IDbConnectionProvider>{}) {}

    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
};

// Alias para compatibilidad con el nombre viejo usado en algunos tests
using MockTournamentRepository = TournamentRepositoryMock;
