#include "controller/TournamentController.hpp"
#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"   // to_json/from_json para Tournament y Groups

#include <algorithm>
#include <sstream>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

// Helpers
static std::string esc(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '"' || c == '\\') { out.push_back('\\'); out.push_back(c); }
        else if (c == '\n') out += "\\n";
        else out.push_back(c);
    }
    return out;
}

static std::string tournament_to_json_string(const domain::Tournament& t) {
    nlohmann::json j = t; // Utilities.hpp
    return j.dump();
}

static domain::Tournament parse_tournament_body(const nlohmann::json& body) {
    domain::Tournament t;
    t = body.get<domain::Tournament>(); // Utilities.hpp
    return t;
}

// POST /tournaments  (con 409 por nombre duplicado)
crow::response TournamentController::CreateTournament(const crow::request& request) {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    try {
        nlohmann::json body = nlohmann::json::parse(request.body);

        // Validación de nombre
        if (!body.contains("name") || !body["name"].is_string()) {
            return crow::response{crow::BAD_REQUEST, "missing 'name'"};
        }
        std::string incomingName = body["name"].get<std::string>();

        // 409 si name ya existe
        auto all = tournamentDelegate->ReadAll();
        auto dup = std::find_if(all.begin(), all.end(),
                                [&](const std::shared_ptr<domain::Tournament>& x){
                                    return x && x->Name() == incomingName;
                                });
        if (dup != all.end()) {
            return crow::response{crow::CONFLICT, "tournament name already exists"};
        }

        domain::Tournament t = parse_tournament_body(body);
        const std::string id = tournamentDelegate->CreateTournament(std::make_shared<domain::Tournament>(t));

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", id);
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        res.write(std::string("{\"id\":\"") + esc(id) + "\"}");
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("create failed: ") + e.what()};
    }
}

// GET /tournaments
crow::response TournamentController::ReadAll() {
    try {
        auto items = tournamentDelegate->ReadAll();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& it : items) if (it) arr.push_back(*it);
        crow::response res{crow::OK, arr.dump()};
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("read all failed: ") + e.what()};
    }
}

// GET /tournaments/{id}  (embebido: groups + teams)
crow::response TournamentController::ReadById(const std::string& id) {
    try {
        auto t = tournamentDelegate->ReadById(id);
        if (!t) return crow::response{crow::NOT_FOUND, "tournament not found"};

        nlohmann::json body = *t; // id, name, format
        // Embebido de grupos:
        auto groups = groupRepository->FindByTournamentId(id);
        body["groups"] = groups;

        crow::response res{crow::OK, body.dump()};
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("read by id failed: ") + e.what()};
    }
}

// PUT /tournaments/{id}
crow::response TournamentController::UpdateTournament(const crow::request& request, const std::string& id) {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    try {
        auto body = nlohmann::json::parse(request.body);
        domain::Tournament t = parse_tournament_body(body);

        if (!tournamentDelegate->UpdateTournament(id, t)) {
            return crow::response{crow::NOT_FOUND, "tournament not found"};
        }
        return crow::response{crow::NO_CONTENT};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("update failed: ") + e.what()};
    }
}

// DELETE /tournaments/{id}
crow::response TournamentController::DeleteTournament(const std::string& id) {
    try {
        if (!tournamentDelegate->DeleteTournament(id)) {
            return crow::response{crow::NOT_FOUND, "tournament not found"};
        }
        return crow::response{crow::NO_CONTENT};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("delete failed: ") + e.what()};
    }
}

// Rutas
REGISTER_ROUTE(TournamentController, ReadAll,          "/tournaments",              "GET"_method)
REGISTER_ROUTE(TournamentController, ReadById,         "/tournaments/<string>",     "GET"_method)
REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments",              "POST"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>",     "PUT"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>",     "DELETE"_method)
