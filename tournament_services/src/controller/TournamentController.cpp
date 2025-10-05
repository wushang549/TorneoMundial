//
// Created by tsuny on 8/31/25.
//
#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <regex>
#include <string>
#include <utility>
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

static const std::regex ID_VALUE("[A-Za-z0-9\\-]+");

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate)
    : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    const std::string id = tournamentDelegate->CreateTournament(tournament);
    crow::response response;
    response.code = crow::CREATED;
    response.add_header("location", id);
    return response;
}

crow::response TournamentController::ReadAll() const {
    nlohmann::json body = tournamentDelegate->ReadAll();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TournamentController::ReadById(const std::string& id) const {
    if (!std::regex_match(id, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (auto t = tournamentDelegate->ReadById(id); t != nullptr) {
        nlohmann::json body = t;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
}

crow::response TournamentController::UpdateTournament(const crow::request& request, const std::string& id) const {
    if (!std::regex_match(id, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    auto body = nlohmann::json::parse(request.body);
    domain::Tournament t = body;

    if (!tournamentDelegate->UpdateTournament(id, t)) {
        return crow::response{crow::NOT_FOUND, "tournament not found"};
    }
    return crow::response{crow::NO_CONTENT};
}

crow::response TournamentController::DeleteTournament(const std::string& id) const {
    if (!std::regex_match(id, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!tournamentDelegate->DeleteTournament(id)) {
        return crow::response{crow::NOT_FOUND, "tournament not found"};
    }
    return crow::response{crow::NO_CONTENT};
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, ReadById, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PUT"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>", "DELETE"_method)
