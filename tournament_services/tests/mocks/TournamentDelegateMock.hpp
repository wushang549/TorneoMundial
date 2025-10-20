#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include "delegate/ITournamentDelegate.hpp"
#include "domain/Tournament.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string&), (override));
    MOCK_METHOD(bool, UpdateTournament, (const std::string&, const domain::Tournament&), (override));
    MOCK_METHOD(bool, DeleteTournament, (const std::string&), (override));
};
