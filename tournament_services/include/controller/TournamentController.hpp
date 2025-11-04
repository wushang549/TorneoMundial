#pragma once
#include <memory>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IGroupRepository.hpp"
class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;
    std::shared_ptr<IGroupRepository>     groupRepository;

public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament,
                                  std::shared_ptr<IGroupRepository> groupRepo)
        : tournamentDelegate(std::move(tournament)), groupRepository(std::move(groupRepo)) {}

    crow::response CreateTournament(const crow::request& request);                   // POST /tournaments
    crow::response ReadAll();                                                       // GET  /tournaments
    crow::response ReadById(const std::string& id);                                 // GET  /tournaments/{id}
    crow::response UpdateTournament(const crow::request& request, const std::string& id); // PUT
    crow::response DeleteTournament(const std::string& id);                              // DELETE
};
