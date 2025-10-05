//
// Created by root on 9/27/25.
//
#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>
#include <regex>

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {
    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

// TeamController.cpp
crow::response TeamController::SaveTeam(const crow::request& request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    auto body = nlohmann::json::parse(request.body);

    // If client provides an id, check it first
    std::string clientId;
    if (body.contains("id") && body["id"].is_string()) {
        clientId = body["id"].get<std::string>();
        if (!clientId.empty()) {
            // ID format check (permite letras, números y guiones)
            if (!std::regex_match(clientId, ID_VALUE)) {
                return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
            }
            // Conflict if already exists
            if (auto existing = teamDelegate->GetTeam(clientId); existing != nullptr) {
                return crow::response{crow::CONFLICT, "team id already exists"};
            }
        }
    }

    try {
        domain::Team team = body;     // must map id if present
        auto createdId = teamDelegate->SaveTeam(team);

        // Prefer the client id if he sent one; else use repository's returned id
        std::string location = !clientId.empty() ? clientId : std::string(createdId);

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", location);
        // add_cors(res); // si ya tienes helper CORS
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("error creating team: ") + e.what()};
    }
}


crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    auto body = nlohmann::json::parse(request.body);
    domain::Team team = body;

    if (!teamDelegate->UpdateTeam(teamId, team)) {
        return crow::response{crow::NOT_FOUND, "team not found"};
    }
    return crow::response{crow::NO_CONTENT};
}

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    try {
        const bool deleted = teamDelegate->DeleteTeam(teamId);
        if (!deleted) {
            // Delete did not apply (likely affected 0 rows / no commit / FK)
            return crow::response{crow::INTERNAL_SERVER_ERROR, "delete not applied"};
        }
        crow::response res{crow::NO_CONTENT};
        // add_cors(res); // if you enabled CORS
        return res;
    } catch (const std::exception& e) {
        // Optional: map FK constraint to 409 if your DB layer exposes it
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("delete failed: ") + e.what()};
    }
}


REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PUT"_method)
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
