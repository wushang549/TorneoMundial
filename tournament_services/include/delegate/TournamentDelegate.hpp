#pragma once
#include <memory>
#include <string>
#include <vector>
#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"

class TournamentDelegate : public ITournamentDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;

public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository)
        : tournamentRepository(std::move(repository)) {}

    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
    std::shared_ptr<domain::Tournament> ReadById(const std::string& id) override;
    bool UpdateTournament(const std::string& id, const domain::Tournament& t) override;
    bool DeleteTournament(const std::string& id) override;
};
