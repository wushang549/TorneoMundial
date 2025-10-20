#pragma once
#include <memory>
#include <string>
#include <vector>
#include "IRepository.hpp"
#include "domain/Tournament.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

class TournamentRepository : public IRepository<domain::Tournament, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit TournamentRepository(std::shared_ptr<IDbConnectionProvider> provider);

    std::string Create(const domain::Tournament& entity) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
    std::shared_ptr<domain::Tournament> ReadById(std::string id) override;
    std::string Update(const domain::Tournament& entity) override;
    void Delete(std::string id) override;
};
