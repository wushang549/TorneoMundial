// GroupController.cpp
// ----------------------------------------------------------
// Controlador REST para endpoints relacionados con grupos
// ----------------------------------------------------------

#include "controller/GroupController.hpp"
#include <crow.h>
#include <nlohmann/json.hpp>
#include <expected>
#include <string_view>
#include <utility>
#include "domain/Utilities.hpp"

using nlohmann::json;
using namespace std::literals;

namespace {
constexpr std::string_view kJsonContentType = "application/json";

inline std::expected<json, std::string> parse_json_body(const std::string& body) {
    auto parsed = json::parse(body, nullptr, false);
    if (parsed.is_discarded()) {
        return std::unexpected("invalid json");
    }
    return parsed;
}

inline crow::response json_error(int status, std::string_view message) {
    crow::response res{status, json{{"error", message}}.dump()};
    res.set_header("content-type", std::string(kJsonContentType));
    return res;
}

inline std::expected<domain::Group, std::string>
build_group_payload(const json& body, const std::string& tournamentId) {
    if (!body.contains("name") || !body["name"].is_string()) {
        return std::unexpected("missing group name");
    }

    domain::Group group;
    group.Name() = body["name"].get<std::string>();
    group.TournamentId() = tournamentId;

    if (body.contains("teams")) {
        if (!body["teams"].is_array()) {
            return std::unexpected("invalid teams collection");
        }
        for (const auto& entry : body["teams"]) {
            if (!entry.contains("id") || !entry["id"].is_string()) {
                return std::unexpected("missing team id");
            }
            if (!entry.contains("name") || !entry["name"].is_string()) {
                return std::unexpected("missing team name");
            }
            domain::Team team{entry["id"].get<std::string>(), entry["name"].get<std::string>()};
            group.Teams().push_back(team);
        }
    }

    return group;
}

inline std::expected<std::string, std::string> extract_team_id(const json& body) {
    if (!body.contains("id") || !body["id"].is_string()) {
        return std::unexpected("missing team id");
    }
    return body["id"].get<std::string>();
}

inline bool is_not_found_reason(std::string_view message) {
    return message == "Tournament doesn't exist" ||
           message == "Group doesn't exist" ||
           message == "Team doesn't exist";
}

inline bool is_conflict_reason(std::string_view message) {
    return message.find("already") != std::string_view::npos ||
           message.find("full") != std::string_view::npos ||
           message.find("capacity") != std::string_view::npos;
}

inline std::expected<std::vector<domain::Team>, std::string>
parse_team_array(const json& body) {
    if (!body.is_array()) {
        return std::unexpected("body must be array");
    }

    std::vector<domain::Team> teams;
    teams.reserve(body.size());
    for (const auto& entry : body) {
        if (!entry.is_object()) {
            return std::unexpected("invalid team payload");
        }

        std::string id;
        if (entry.contains("id") && entry["id"].is_string()) {
            id = entry["id"].get<std::string>();
        } else if (entry.contains("Id") && entry["Id"].is_string()) {
            id = entry["Id"].get<std::string>();
        } else {
            return std::unexpected("missing team id");
        }

        std::string name;
        if (entry.contains("name") && entry["name"].is_string()) {
            name = entry["name"].get<std::string>();
        }

        teams.push_back(domain::Team{std::move(id), std::move(name)});
    }

    return teams;
}

// Helper to publish an event when a team is added. For now make it a no-op
void publish_team_added_event(const std::string& /*tournamentId*/, const std::string& /*groupId*/, const std::string& /*teamId*/) {}

} // namespace

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    auto result = groupDelegate->GetGroup(tournamentId, groupId);
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        return json_error(crow::BAD_REQUEST, result.error());
    }

    const auto& groupPtr = result.value();
    if (!groupPtr) {
        return json_error(crow::NOT_FOUND, "group not found");
    }

    json body;
    domain::to_json(body, groupPtr); // use to_json from Utilities.hpp
    crow::response res{crow::OK, body.dump()};
    res.set_header("content-type", std::string(kJsonContentType));
    return res;
}


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
        return json_error(crow::NOT_FOUND, result.error());
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

    crow::response res{crow::OK, arr.dump()};
    res.set_header("content-type", std::string(kJsonContentType));
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
    auto payload = parse_json_body(req.body);
    if (!payload) {
        return json_error(crow::BAD_REQUEST, payload.error());
    }

    auto groupPayload = build_group_payload(*payload, tournamentId);
    if (!groupPayload) {
        return json_error(crow::BAD_REQUEST, groupPayload.error());
    }

    auto result = groupDelegate->CreateGroup(tournamentId, groupPayload.value());
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        return json_error(crow::CONFLICT, result.error());
    }

    json responseBody{{"id", result.value()}, {"name", groupPayload->Name()}};
    crow::response res{crow::CREATED, responseBody.dump()};
    res.set_header("content-type", std::string(kJsonContentType));
    res.set_header("location", result.value());
    return res;
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
    auto payload = parse_json_body(req.body);
    if (!payload) {
        return json_error(crow::BAD_REQUEST, payload.error());
    }

    auto teamId = extract_team_id(*payload);
    if (!teamId) {
        return json_error(crow::BAD_REQUEST, teamId.error());
    }

    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamId.value());
    if (!result) {
        const auto& err = result.error();
        if (is_not_found_reason(err)) {
            return json_error(crow::NOT_FOUND, err);
        }
        if (is_conflict_reason(err)) {
            return json_error(crow::CONFLICT, err);
        }
        return json_error(crow::BAD_REQUEST, err);
    }

    crow::response res{crow::CREATED, json{{"message", "Team added successfully"}}.dump()};
    res.set_header("content-type", std::string(kJsonContentType));
    publish_team_added_event(tournamentId, groupId, teamId.value());
    return res;
}

crow::response GroupController::AddTeamToGroupById(const std::string& tournamentId,
                                                   const std::string& groupId,
                                                   const std::string& teamId) {
    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamId);
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        if (is_conflict_reason(result.error())) {
            return json_error(crow::CONFLICT, result.error());
        }
        return json_error(crow::BAD_REQUEST, result.error());
    }

    publish_team_added_event(tournamentId, groupId, teamId);
    return crow::response{crow::NO_CONTENT};
}

crow::response GroupController::UpdateTeams(const crow::request& req,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    auto payload = parse_json_body(req.body);
    if (!payload) {
        return json_error(crow::BAD_REQUEST, payload.error());
    }

    auto teamsPayload = parse_team_array(*payload);
    if (!teamsPayload) {
        return json_error(crow::BAD_REQUEST, teamsPayload.error());
    }

    auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teamsPayload.value());
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        if (is_conflict_reason(result.error())) {
            return json_error(crow::CONFLICT, result.error());
        }
        return json_error(crow::BAD_REQUEST, result.error());
    }

    return crow::response{crow::NO_CONTENT};
}

crow::response GroupController::RenameGroup(const crow::request& req,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    auto payload = parse_json_body(req.body);
    if (!payload) {
        return json_error(crow::BAD_REQUEST, payload.error());
    }

    if (!payload->contains("name") || !(*payload)["name"].is_string()) {
        return json_error(crow::BAD_REQUEST, "missing group name");
    }

    domain::Group group;
    group.Id() = groupId;
    group.Name() = (*payload)["name"].get<std::string>();

    auto result = groupDelegate->UpdateGroup(tournamentId, group);
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        return json_error(crow::CONFLICT, result.error());
    }

    return crow::response{crow::NO_CONTENT};
}

crow::response GroupController::DeleteGroup(const std::string& tournamentId,
                                            const std::string& groupId) {
    auto result = groupDelegate->RemoveGroup(tournamentId, groupId);
    if (!result) {
        if (is_not_found_reason(result.error())) {
            return json_error(crow::NOT_FOUND, result.error());
        }
        return json_error(crow::CONFLICT, result.error());
    }

    return crow::response{crow::NO_CONTENT};
}

// ============= ROUTE REGISTRATION (Registry/Macros) =============
REGISTER_ROUTE(GroupController, GetGroups,      "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup,       "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup,    "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateTeams,    "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeamToGroupById, "/tournaments/<string>/groups/<string>/teams/<string>", "POST"_method)
REGISTER_ROUTE(GroupController, AddTeamToGroup, "/tournaments/<string>/groups/<string>", "POST"_method)
REGISTER_ROUTE(GroupController, RenameGroup,    "/tournaments/<string>/groups/<string>", "PUT"_method)
REGISTER_ROUTE(GroupController, DeleteGroup,    "/tournaments/<string>/groups/<string>", "DELETE"_method)
