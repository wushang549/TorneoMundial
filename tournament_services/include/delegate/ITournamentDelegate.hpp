#pragma once
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include "domain/Tournament.hpp"

struct ITournamentDelegate {
    virtual ~ITournamentDelegate() = default;

    virtual std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>
    ReadAll() = 0;

    virtual std::expected<std::shared_ptr<domain::Tournament>, std::string>
    ReadById(const std::string& id) = 0;

    virtual std::expected<std::string, std::string>
    CreateTournament(std::shared_ptr<domain::Tournament> t) = 0;

    virtual std::expected<bool, std::string>
    UpdateTournament(const std::string& id, const domain::Tournament& t) = 0;

    virtual std::expected<bool, std::string>
    DeleteTournament(const std::string& id) = 0;
};
