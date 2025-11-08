//IMatchRepository.hpp
#pragma once
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include "domain/Match.hpp"

/**
 * Contract for match persistence.
 * Aligns with patterns used by TeamRepository/TournamentRepository.
 */
class IMatchRepository {
public:
    virtual ~IMatchRepository() = default;

    // Read all matches for a tournament (optionally you can post-filter at delegate).
    virtual std::vector<std::shared_ptr<domain::Match>>
    FindByTournamentId(const std::string& tournamentId) = 0;

    // Read a single match by composite (tournamentId, matchId).
    virtual std::shared_ptr<domain::Match>
    FindByTournamentIdAndMatchId(const std::string& tournamentId,
                                 const std::string& matchId) = 0;

    // Update the full document for an existing match.
    // Returns the id or throws on not found / failure.
    virtual std::string Update(const domain::Match& entity) = 0;

    // Optional (not used by current controller): create/delete helpers
    // virtual std::string Create(const domain::Match& entity) = 0;
    // virtual void Delete(const std::string& tournamentId, const std::string& matchId) = 0;
};
