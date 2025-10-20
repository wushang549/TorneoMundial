// GroupController.cpp
// ----------------------------------------------------------
// Controlador REST para endpoints relacionados con grupos
// ----------------------------------------------------------

#include "controller/GroupController.hpp"
#include <crow.h>
#include <nlohmann/json.hpp>

using nlohmann::json;
using namespace std::literals;

// Constructor
GroupController::GroupController(const std::shared_ptr<GroupDelegate>& groupDelegate)
    : groupDelegate(groupDelegate) {}

/* 
   GET /tournaments/<TOURNAMENT-ID>/groups
   Devuelve los grupos de un torneo.
   Respuestas:
     200 -> Lista JSON con grupos y equipos
     404 -> Si el torneo no existe o error de consulta
    */
crow::response GroupController::GetGroups(const std::string& tournamentId) {
    auto result = groupDelegate->GetGroups(tournamentId);
    if (!result.has_value()) {
        return crow::response(crow::NOT_FOUND, result.error());
    }

    json arr = json::array();
    for (const auto& g : result.value()) {
        json groupJson;
        groupJson["name"] = g->Name();
        groupJson["id"]   = g->Id();

        json teamArr = json::array();
        for (const auto& t : g->Teams()) {
            teamArr.push_back({{"id", t.Id}, {"name", t.Name}});
        }
        groupJson["teams"] = teamArr;
        arr.push_back(groupJson);
    }

    crow::response res{crow::OK};
    res.set_header("content-type", "application/json");
    res.body = arr.dump();
    return res;
}

/* 
   POST /tournaments/<TOURNAMENT-ID>/groups
   Crea un nuevo grupo dentro de un torneo.
   Cuerpo JSON:
     {
       "name": "Group A",
       "teams": [ { "id": "team-id", "name": "team name" } ]
     }
   Respuestas:
     201 -> Grupo creado
     404 -> Torneo no existe o error al crear
    */
crow::response GroupController::CreateGroup(const crow::request& req,
                                            const std::string& tournamentId) {
    try {
        json body = json::parse(req.body);
        if (!body.contains("name")) {
            return crow::response(crow::BAD_REQUEST, "Missing group name");
        }

        domain::Group group;
        group.Name() = body["name"].get<std::string>();
        group.TournamentId() = tournamentId;

        if (body.contains("teams")) {
            for (auto& t : body["teams"]) {
                domain::Team team{
                    t.value("id", ""), 
                    t.value("name", "")
                };
                group.Teams().push_back(team);
            }
        }

        auto result = groupDelegate->CreateGroup(tournamentId, group);
        if (!result.has_value()) {
            if (result.error().find("Tournament") != std::string::npos)
                return crow::response(crow::NOT_FOUND, result.error());
            return crow::response(crow::CONFLICT, result.error());
        }

        json j{{"id", result.value()}, {"name", group.Name()}};
        crow::response res{crow::CREATED};
        res.set_header("content-type", "application/json");
        res.set_header("location", result.value());
        res.body = j.dump();
        return res;
    } catch (const std::exception& e) {
        return crow::response(crow::BAD_REQUEST, e.what());
    }
}

/* 
   POST /tournaments/<TOURNAMENT-ID>/groups/<GROUP-ID>
   Agrega un equipo a un grupo específico dentro de un torneo.
   Cuerpo JSON:
     { "id": "team-id", "name": "team name" }
   Respuestas:
     201 -> Equipo agregado
     404 -> Torneo, grupo o equipo inexistente
     409 -> Grupo lleno o conflicto de validación
    */
crow::response GroupController::AddTeamToGroup(const crow::request& req,
                                               const std::string& tournamentId,
                                               const std::string& groupId) {
    try {
        json body = json::parse(req.body);
        if (!body.contains("id") || !body.contains("name")) {
            return crow::response(crow::BAD_REQUEST, "Missing team id or name");
        }

        auto teamId = body["id"].get<std::string>();
        auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamId);

        if (!result.has_value()) {
            const auto& err = result.error();
            if (err.find("Tournament") != std::string::npos ||
                err.find("Group") != std::string::npos ||
                err.find("Team doesn't exist") != std::string::npos) {
                return crow::response(crow::NOT_FOUND, err);
            }
            if (err.find("full") != std::string::npos ||
                err.find("already") != std::string::npos) {
                return crow::response(crow::CONFLICT, err);
            }
            return crow::response(crow::BAD_REQUEST, err);
        }

        // Procesamiento de fondo (evento) puede dispararse aquí.
        crow::response res{crow::CREATED};
        res.set_header("content-type", "application/json");
        res.body = R"({"message":"Team added successfully"})";
        return res;
    } catch (const std::exception& e) {
        return crow::response(crow::BAD_REQUEST, e.what());
    }
}
