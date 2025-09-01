//
// Created by tsuny on 8/31/25.
//

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <utility>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> tournamentDelegate) : tournamentDelegate(std::move(tournamentDelegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    nlohmann::json body = nlohmann::json::parse(request.body);
    std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    std::string_view id = tournamentDelegate->CreateTournament(tournament);
    crow::response response;
    response.code = crow::CREATED;
    response.add_header("location", id.data());
    return response;
}

// REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
// REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)