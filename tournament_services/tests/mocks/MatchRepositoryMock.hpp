#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "persistence/repository/IMatchRepository.hpp"
#include "domain/Match.hpp"

class MatchRepositoryMock : public IMatchRepository {
public:
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>,
                FindByTournamentId,
                (const std::string&),
                (override));

    MOCK_METHOD(std::shared_ptr<domain::Match>,
                FindByTournamentIdAndMatchId,
                (const std::string&, const std::string&),
                (override));

    MOCK_METHOD(std::string,
                Update,
                (const domain::Match&),
                (override));

    MOCK_METHOD(std::string,
                Create,
                (const domain::Match&),
                (override));

    MOCK_METHOD(std::string,
                CreateIfNotExists,
                (const domain::Match&),
                (override));
};
