#pragma once
#include <memory>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "delegate/ITournamentDelegate.hpp"

class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;

public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament);

    crow::response CreateTournament(const crow::request& request);                   // POST /tournaments
    crow::response ReadAll();                                                       // GET  /tournaments
    crow::response ReadById(const std::string& id);                                 // GET  /tournaments/{id}
    crow::response UpdateTournament(const crow::request& request, const std::string& id); // PUT
    crow::response DeleteTournament(const std::string& id);                              // DELETE
};
