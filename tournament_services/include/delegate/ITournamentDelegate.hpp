#pragma once
#include <memory>
#include <string>
#include <vector>
#include "domain/Tournament.hpp"

struct ITournamentDelegate {
    virtual ~ITournamentDelegate() = default;

    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;
    virtual std::shared_ptr<domain::Tournament> ReadById(const std::string& id) = 0;

    virtual std::string CreateTournament(std::shared_ptr<domain::Tournament> t) = 0;
    virtual bool UpdateTournament(const std::string& id, const domain::Tournament& t) = 0;
    virtual bool DeleteTournament(const std::string& id) = 0;
};
