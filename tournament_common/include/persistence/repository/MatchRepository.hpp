#pragma once
#include <memory>
#include <vector>
#include <string>
#include <pqxx/pqxx>
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

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

    std::string Create(const domain::Match& entity) override;
    std::string CreateIfNotExists(const domain::Match& entity) override;
    std::string Update(const domain::Match& entity) override;
};
