//MatchRepository.hpp
#pragma once
#include <memory>
#include <vector>
#include <string>
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

/**
 * PostgreSQL implementation for match persistence.
 * Stores the match JSON in a "document" jsonb column, similar to tournaments/groups.
 *
 * Expected schema (migration suggestion):
 *   CREATE TABLE matches (
 *     id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
 *     tournament_id UUID NOT NULL REFERENCES tournaments(id) ON DELETE CASCADE,
 *     document JSONB NOT NULL,
 *     created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
 *     last_update_date TIMESTAMPTZ
 *   );
 *   CREATE INDEX idx_matches_tournament ON matches(tournament_id);
 */
class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

    static std::string to_doc_string(const domain::Match& m);
    static std::shared_ptr<domain::Match> row_to_domain(const pqxx::row& row);

public:
    explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> provider);

    std::vector<std::shared_ptr<domain::Match>>
    FindByTournamentId(const std::string& tournamentId) override;

    std::shared_ptr<domain::Match>
    FindByTournamentIdAndMatchId(const std::string& tournamentId,
                                 const std::string& matchId) override;

    std::string Update(const domain::Match& entity) override;
};
