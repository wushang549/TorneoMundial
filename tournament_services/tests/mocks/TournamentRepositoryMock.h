#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"

using TournamentRepoIface = IRepository<domain::Tournament, std::string>;

class MockTournamentRepository : public TournamentRepoIface {
public:
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
};
