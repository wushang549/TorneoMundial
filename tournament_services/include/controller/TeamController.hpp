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
#include <utility>

#include "configuration/RouteDefinition.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"

static const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;
public:
    explicit TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

    [[nodiscard]] crow::response getTeam(const std::string& teamId) const {
        if(!std::regex_match(teamId, ID_VALUE)) {
            return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
        }

        if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
            nlohmann::json body = team;
            auto response = crow::response{crow::OK, body.dump()};
            response.add_header("Content-Type", "application/json");
            return response;
        }
        return crow::response{crow::NOT_FOUND, "team not found"};
    }

    [[nodiscard]] crow::response getAllTeams() const {

        nlohmann::json body = teamDelegate->GetAllTeams();
        return crow::response{200, body.dump()};
    }

    [[nodiscard]] crow::response SaveTeam(const crow::request& request) const {
        crow::response response;
        
        if(!nlohmann::json::accept(request.body)) {
            response.code = crow::BAD_REQUEST;
            return response;
        }
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Team team = requestBody;

        auto createdId = teamDelegate->SaveTeam(team);
        response.code = crow::CREATED;
        response.add_header("location", createdId.data());

        return response;
    }
};

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
#endif //RESTAPI_TEAM_CONTROLLER_HPP