#pragma once
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include "domain/Match.hpp"

class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    virtual std::vector<std::shared_ptr<domain::Match>>
    FindByTournamentId(const std::string& tournamentId) = 0;

    virtual std::shared_ptr<domain::Match>
    FindByTournamentIdAndMatchId(const std::string& tournamentId,
                                 const std::string& matchId) = 0;

    // Create (may throw on UNIQUE violation if caller no filtra)
    virtual std::string Create(const domain::Match& entity) = 0;

    // Idempotent insert: returns existing id when duplicate key
    virtual std::string CreateIfNotExists(const domain::Match& entity) = 0;

    virtual std::string Update(const domain::Match& entity) = 0;
};
