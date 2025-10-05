//
// Created by tsuny on 8/31/25.
//
#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string>
#include <memory>
#include <vector>
#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;

    virtual std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;                // Create
    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;                                   // Read all

    // New for full CRUD:
    virtual std::shared_ptr<domain::Tournament> ReadById(const std::string& id) = 0;                          // Read by id
    virtual bool UpdateTournament(const std::string& id, const domain::Tournament& tournament) = 0;           // Update
    virtual bool DeleteTournament(const std::string& id) = 0;                                                 // Delete
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
