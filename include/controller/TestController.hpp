//
// Created by developer on 8/22/25.
//

#ifndef RESTAPI_TEAMCONTROLLER_HPP
#define RESTAPI_TEAMCONTROLLER_HPP
#include <string>

#include "configuration/RouteDefinition.hpp"

class TestController {

public:
    std::string getTest(const std::string testId) {
        return testId;
    }

    std::string getTests() {
        return "all teams";
    }

    std::string saveTests(const crow::request& request) {

        return "trying to save a team";
    }
};

REGISTER_ROUTE(TestController, getTest, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TestController, getTests, "/teams", "GET"_method)
REGISTER_ROUTE(TestController, saveTests, "/teams", "POST"_method)
#endif //RESTAPI_TEAMCONTROLLER_HPP