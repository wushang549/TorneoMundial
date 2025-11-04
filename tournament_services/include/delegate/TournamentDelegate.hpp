#pragma once
#include <expected>
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

    std::expected<std::string, std::string>
    CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;

    std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>
    ReadAll() override;

    std::expected<std::shared_ptr<domain::Tournament>, std::string>
    ReadById(const std::string& id) override;

    std::expected<bool, std::string>
    UpdateTournament(const std::string& id, const domain::Tournament& t) override;

    std::expected<bool, std::string>
    DeleteTournament(const std::string& id) override;
};
