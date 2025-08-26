//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/TeamDelegate.hpp"
#include "domain/Team.hpp"

namespace domain {
    inline void to_json(nlohmann::json& json, const Team& team) {
        json = {{"id", team.Id}, {"name", team.Name}};
    }
}
class TeamController {
    std::shared_ptr<TeamDelegate> teamDelegate;
public:
    TeamController(std::shared_ptr<TeamDelegate> teamDelegate) : teamDelegate(teamDelegate) {}

    crow::response getTest(const std::string testId) {
        domain::Team team;
        team.Id = testId;

        nlohmann::json body;
        body["name"] = team.Name;
        body["id"] = team.Id;


        return crow::response{200, body.dump()};
    }

    crow::response getAllTeams() {

        nlohmann::json body = teamDelegate->getAllTeams();
        return crow::response{200, body.dump()};
    }

    std::string saveTests(const crow::request& request) {

        return "trying to save a team";
    }
};


REGISTER_ROUTE(TeamController, getTest, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, saveTests, "/teams", "POST"_method)
#endif //RESTAPI_TEAM_CONTROLLER_HPP