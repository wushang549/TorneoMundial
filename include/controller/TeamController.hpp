//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAM_CONTROLLER_HPP
#define RESTAPI_TEAM_CONTROLLER_HPP

#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "domain/Team.hpp"

class TeamController {
// domain::Team team;

public:
    crow::response getTest(const std::string testId) {
        domain::Team team;
        team.Id = testId;

        nlohmann::json body;
        body["name"] = team.Name;
        body["id"] = team.Id;


        return crow::response{200, body.dump()};
    }

    std::string getTests() {
        return "all teams";
    }

    std::string saveTests(const crow::request& request) {

        return "trying to save a team";
    }
};


REGISTER_ROUTE(TeamController, getTest, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getTests, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, saveTests, "/teams", "POST"_method)
#endif //RESTAPI_TEAM_CONTROLLER_HPP