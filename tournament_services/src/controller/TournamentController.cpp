#include "controller/TournamentController.hpp"
#include "configuration/RouteDefinition.hpp"
#include <utility>
#include <sstream>

// enum helpers
static std::string type_to_string(domain::TournamentType t) {
    return (t == domain::TournamentType::NFL) ? "NFL" : "ROUND_ROBIN";
}
static domain::TournamentType string_to_type(const std::string& s) {
    return (s == "NFL") ? domain::TournamentType::NFL : domain::TournamentType::ROUND_ROBIN;
}

// tiny escaper
static std::string esc(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\' || c == '\"') { out.push_back('\\'); out.push_back(c); }
        else out.push_back(c);
    }
    return out;
}

// body -> domain
static domain::Tournament parse_tournament_body(const nlohmann::json& j) {
    const std::string name = j.value("name", "");
    const auto& jf = j.at("format");
    const int ng   = jf.value("numberOfGroups", 1);
    const int mtg  = jf.value("maxTeamsPerGroup", 16);
    const std::string ts = jf.value("type", "ROUND_ROBIN");
    domain::TournamentFormat fmt{ng, mtg, string_to_type(ts)};
    return domain::Tournament{name, fmt};
}

// domain -> json text
static std::string tournament_to_json_string(const domain::Tournament& t) {
    std::string s;
    s.reserve(128);
    s += "{\"id\":\"";    s += esc(t.Id());   s += "\"";
    s += ",\"name\":\"";  s += esc(t.Name()); s += "\"";
    s += ",\"format\":{";
    s += "\"numberOfGroups\":";   s += std::to_string(t.Format().NumberOfGroups());
    s += ",\"maxTeamsPerGroup\":"; s += std::to_string(t.Format().MaxTeamsPerGroup());
    s += ",\"type\":\"";  s += type_to_string(t.Format().Type()); s += "\"}";
    s += "}";
    return s;
}

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> tournament)
    : tournamentDelegate(std::move(tournament)) {}

// POST /tournaments
crow::response TournamentController::CreateTournament(const crow::request& request) {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    try {
        nlohmann::json body = nlohmann::json::parse(request.body);
        domain::Tournament t = parse_tournament_body(body);
        const std::string id = tournamentDelegate->CreateTournament(std::make_shared<domain::Tournament>(t));

        crow::response res;
        res.code = crow::CREATED;
        res.add_header("location", id);
        res.add_header("content-type", "application/json");
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
        std::string payload;
        payload.reserve(items.size() * 128 + 2);
        payload.push_back('[');
        bool first = true;
        for (const auto& it : items) {
            if (!first) payload.push_back(',');
            first = false;
            payload += tournament_to_json_string(*it);
        }
        payload.push_back(']');
        crow::response res{crow::OK, payload};
        res.add_header("content-type", "application/json");
        return res;
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string("read all failed: ") + e.what()};
    }
}

// GET /tournaments/{id}
crow::response TournamentController::ReadById(const std::string& id) {
    try {
        auto t = tournamentDelegate->ReadById(id);
        if (!t) return crow::response{crow::NOT_FOUND, "tournament not found"};
        std::string body = tournament_to_json_string(*t);
        crow::response res{crow::OK, body};
        res.add_header("content-type", "application/json");
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
        nlohmann::json body = nlohmann::json::parse(request.body);
        domain::Tournament t = parse_tournament_body(body);
        t.Id() = id;

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

// Route registrations
REGISTER_ROUTE(TournamentController, ReadAll,          "/tournaments",              "GET"_method)
REGISTER_ROUTE(TournamentController, ReadById,         "/tournaments/<string>",     "GET"_method)
REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments",              "POST"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>",     "PUT"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>",     "DELETE"_method)
