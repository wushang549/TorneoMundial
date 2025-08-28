//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <regex>

#include "configuration/RouteDefinition.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "domain/Team.hpp"

namespace domain {
    inline void to_json(nlohmann::json& json, const Team& team) {
        json = {{"id", team.Id}, {"name", team.Name}};
    }

    inline void from_json(const nlohmann::json& json, Team& team) {
        json.at("id").get_to(team.Id);
        json.at("name").get_to(team.Name);
    }

    inline void to_json(nlohmann::json& json, std::shared_ptr<Team> team) {
        json = {{"id", team->Id}, {"name", team->Name}};
    }
}

static const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;
public:
    TeamController(std::shared_ptr<ITeamDelegate> teamDelegate) : teamDelegate(teamDelegate) {}

    crow::response getTeam(const std::string teamId) {
        if(!std::regex_match(teamId, ID_VALUE)) {
            return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
        }

        auto team = teamDelegate->GetTeam(teamId);

        if(team != nullptr) {
            nlohmann::json body = team;
            return crow::response{crow::OK, body.dump()};
        }
        return crow::response{crow::NOT_FOUND, "team not found"};
    }

    crow::response getAllTeams() {

        nlohmann::json body = teamDelegate->GetAllTeams();
        return crow::response{200, body.dump()};
    }

    crow::response SaveTeam(const crow::request& request) {
        crow::response response;
        
        if(nlohmann::json::accept(request.body)) {
            response.code = crow::BAD_REQUEST;
            return response;
        }
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Team team = requestBody;

        teamDelegate->SaveTeam(team);
        response.code = crow::CREATED;

        return response;
    }
};


REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
#endif //RESTAPI_TEAM_CONTROLLER_HPP