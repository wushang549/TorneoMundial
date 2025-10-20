//
// Created by root on 9/27/25.
//
#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <regex>

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

// Obtenemos el team por el ID
crow::response TeamController::getTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto team = teamDelegate->GetTeam(teamId);
    if (team == nullptr) {
        return crow::response{crow::NOT_FOUND, "team not found"};
    }

    nlohmann::json body = *team;
    auto response = crow::response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

//Obtenemos todos los teams por /teams
crow::response TeamController::getAllTeams() const {
    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{crow::OK, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

// POST /teams  (con 409 por nombre duplicado)
crow::response TeamController::SaveTeam(const crow::request& request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    auto body = nlohmann::json::parse(request.body);

    // Validación de nombre
    if (!body.contains("name") || !body["name"].is_string()) {
        return crow::response{crow::BAD_REQUEST, "missing 'name'"};
    }
    std::string incomingName = body["name"].get<std::string>();

    // 409 si name ya existe
    auto all = teamDelegate->GetAllTeams();
    auto dup = std::find_if(all.begin(), all.end(),
                            [&](const std::shared_ptr<domain::Team>& x){
                                return x && x->Name == incomingName;
                            });
    if (dup != all.end()) {
        return crow::response{crow::CONFLICT, "team name already exists"};
    }

    // Si el cliente pasa id, valida formato y conflicto de id
    std::string clientId;
    if (body.contains("id") && body["id"].is_string()) {
        clientId = body["id"].get<std::string>();
        if (!std::regex_match(clientId, ID_VALUE)) {
            return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
        }
        if (auto existing = teamDelegate->GetTeam(clientId); existing != nullptr) {
            return crow::response{crow::CONFLICT, "team id already exists"};
        }
    }

    try {
        domain::Team team = body;
        auto createdId = teamDelegate->SaveTeam(team);

        std::string location = !clientId.empty() ? clientId : std::string(createdId);

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", location);
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);   
        res.write(nlohmann::json{{"id", location}}.dump());
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("error creating team: ") + e.what()};
    }
}

//PATCH con validacon de 404 (en caso de que no exista en la base de datos)
crow::response TeamController::UpdateTeam(const crow::request& request,
                                          const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }

    nlohmann::json body = nlohmann::json::parse(request.body);

    domain::Team team = body;  // id en body es opcional/ignorado
    team.Id = teamId;     

    const bool updated = teamDelegate->UpdateTeam(teamId, team);
    if (!updated) {
        //equipo no encontrado 44
        return crow::response{crow::NOT_FOUND, "team not found"};
    }
    return crow::response{crow::NO_CONTENT}; // 204
}

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if (!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    try {
        const bool deleted = teamDelegate->DeleteTeam(teamId);
        if (!deleted) {
            return crow::response{crow::NOT_FOUND, "team not found"};
        }
        crow::response res;
        res.code = crow::NO_CONTENT;
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("delete failed: ") + e.what()};
    }
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PUT"_method)
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
