//
// Created by tsuny on 8/31/25.
//
#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <memory>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/ITournamentDelegate.hpp"

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;

public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament);

    [[nodiscard]] crow::response CreateTournament(const crow::request &request) const; // POST /tournaments
    [[nodiscard]] crow::response ReadAll() const;                                      // GET  /tournaments

    // New endpoints:
    [[nodiscard]] crow::response ReadById(const std::string& id) const;                // GET  /tournaments/{id}
    [[nodiscard]] crow::response UpdateTournament(const crow::request& request, const std::string& id) const; // PUT
    [[nodiscard]] crow::response DeleteTournament(const std::string& id) const;         // DELETE
};

#endif //TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
