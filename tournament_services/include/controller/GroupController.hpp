#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

// ====== RESPONSES ======
#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <vector>
#include <string>
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

class GroupController {
    std::shared_ptr<IGroupDelegate> groupDelegate;

    // Hook opcional para eventos (no hace nada por ahora)
    void publish_team_added_event(const std::string& tournamentId,
                                  const std::string& groupId,
                                  const std::string& teamId) {
        (void)tournamentId; (void)groupId; (void)teamId;
    }

public:
    explicit GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
        : groupDelegate(delegate) {}
    ~GroupController() = default;

    // GET /tournaments/{tid}/groups
    crow::response GetGroups(const std::string& tournamentId);

    // GET /tournaments/{tid}/groups/{gid}
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);

    // POST /tournaments/{tid}/groups
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);

    // (stub original, lo dejamos sin tocar)
    crow::response UpdateGroup(const crow::request& request);

    // PATCH /tournaments/{tid}/groups/{gid}/teams  (batch)
    crow::response UpdateTeams(const crow::request& request,
                               const std::string& tournamentId,
                               const std::string& groupId);

    // POST /tournaments/{tid}/groups/{gid}/teams/{teamId}
    crow::response AddTeamToGroup(const std::string& tournamentId,
                                  const std::string& groupId,
                                  const std::string& teamId);

    // POST /tournaments/{tid}/groups/{gid}  (equipo en body → 201)
    crow::response AddTeamToGroupFromBody(const crow::request& request,
                                          const std::string& tournamentId,
                                          const std::string& groupId);

    // === NUEVOS ===
    // PUT /tournaments/{tid}/groups/{gid}  (renombrar grupo)
    crow::response RenameGroup(const crow::request& request,
                               const std::string& tournamentId,
                               const std::string& groupId);

    // DELETE /tournaments/{tid}/groups/{gid}
    crow::response DeleteGroup(const std::string& tournamentId,
                               const std::string& groupId);
};

// ============= IMPLEMENTATIONS (INLINE) =================

inline crow::response GroupController::GetGroups(const std::string& tournamentId){
    auto groupsExp = groupDelegate->GetGroups(tournamentId);
    if (!groupsExp) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, R"({"error":"db error"})"};
    }

    nlohmann::json body = nlohmann::json::array();
    for (const auto& gp : *groupsExp) {
        if (gp) body.push_back(*gp);
    }

    crow::response r{crow::OK, body.dump()};
    r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return std::move(r);
}

inline crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    auto groupExp = groupDelegate->GetGroup(tournamentId, groupId);
    if (!groupExp) {
        return crow::response{crow::NOT_FOUND, R"({"error":"not found"})"};
    }

    const auto& gp = *groupExp;
    if (!gp) {
        return crow::response{crow::NOT_FOUND, R"({"error":"not found"})"};
    }

    nlohmann::json body = *gp;
    crow::response r{crow::OK, body.dump()};
    r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return std::move(r);
}

inline crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId){
    using nlohmann::json;
    json j;
    try { j = json::parse(request.body); }
    catch (...) {
        return crow::response{crow::BAD_REQUEST, R"({"error":"invalid json"})"};
    }

    if (!j.contains("name") || !j["name"].is_string()) {
        return crow::response{crow::BAD_REQUEST, R"({"error":"missing 'name'"})"};
    }

    domain::Group group;
    group.Name() = j["name"].get<std::string>();

    if (j.contains("teams") && j["teams"].is_array()) {
        for (const auto& t : j["teams"]) {
            if (!t.is_object()) continue;
            domain::Team dt;
            dt.Id = t.value("Id", t.value("id",""));
            if (!dt.Id.empty()) group.Teams().push_back(dt);
        }
    }

    auto idExp = groupDelegate->CreateGroup(tournamentId, group);

    crow::response r;
    if (idExp) {
        r.code = crow::CREATED;
        r.add_header("location", *idExp);
        r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        r.write(nlohmann::json{{"id", *idExp}}.dump()); // 👈 body con id
    } else {
        r.code = 422;
        r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        r.write(nlohmann::json{{"error", idExp.error()}}.dump());
    }
    return std::move(r);
}

inline crow::response GroupController::UpdateGroup(const crow::request&) {
    return crow::response{crow::NOT_IMPLEMENTED};
}

inline crow::response GroupController::UpdateTeams(const crow::request& request,
                                                   const std::string& tournamentId,
                                                   const std::string& groupId) {
    using nlohmann::json;
    json j;
    try { j = json::parse(request.body); }
    catch (...) {
        return crow::response{crow::BAD_REQUEST, R"({"error":"invalid json"})"};
    }
    if (!j.is_array()) {
        return crow::response{crow::BAD_REQUEST, R"({"error":"body must be array"})"};
    }

    std::vector<domain::Team> teams;
    teams.reserve(j.size());
    for (const auto& e : j) {
        if (!e.is_object()) continue;
        domain::Team dt;
        dt.Id = e.value("Id", e.value("id",""));
        if (!dt.Id.empty()) teams.push_back(dt);
    }

    auto res = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (res) return crow::response{crow::NO_CONTENT};

    return crow::response{422, res.error()};
}

inline crow::response GroupController::AddTeamToGroup(const std::string& tournamentId,
                                                      const std::string& groupId,
                                                      const std::string& teamId) {
    auto res = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamId);
    if (res) return crow::response{crow::NO_CONTENT};
    crow::response r{422, res.error()};
    return std::move(r);
}

inline crow::response GroupController::AddTeamToGroupFromBody(const crow::request& request,
                                                              const std::string& tournamentId,
                                                              const std::string& groupId) {
    using nlohmann::json;
    json j;
    try { j = json::parse(request.body); }
    catch (...) { return crow::response{400, R"({"error":"invalid json"})"}; }

    std::string teamId;
    if (j.contains("id") && j["id"].is_string())      teamId = j["id"].get<std::string>();
    else if (j.contains("Id") && j["Id"].is_string()) teamId = j["Id"].get<std::string>();
    if (teamId.empty()) {
        return crow::response{400, R"({"error":"missing 'id'"})"};
    }

    auto res = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamId);
    if (!res) {
        std::string err = res.error();
        int code = (err.find("doesn't exist") != std::string::npos) ? 404 : 422;
        return crow::response{code, err};
    }

    nlohmann::json body;
    body["id"] = teamId;
    if (j.contains("name") && j["name"].is_string()) body["name"] = j["name"].get<std::string>();

    crow::response r{crow::CREATED, body.dump()};
    r.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    publish_team_added_event(tournamentId, groupId, teamId);

    return std::move(r);
}

// === NUEVOS ===

inline crow::response GroupController::RenameGroup(const crow::request& request,
                                                   const std::string& tournamentId,
                                                   const std::string& groupId) {
    using nlohmann::json;
    json j;
    try { j = json::parse(request.body); }
    catch (...) { return crow::response{400, R"({"error":"invalid json"})"}; }

    if (!j.contains("name") || !j["name"].is_string()) {
        return crow::response{400, R"({"error":"missing 'name'"})"};
    }

    domain::Group g;
    g.Id() = groupId;      // forzamos id del path
    g.Name() = j["name"].get<std::string>();

    auto res = groupDelegate->UpdateGroup(tournamentId, g);
    if (!res) {
        std::string err = res.error();
        int code = (err.find("doesn't exist") != std::string::npos) ? 404 : 422;
        return crow::response{code, err};
    }
    return crow::response{crow::NO_CONTENT};
}

inline crow::response GroupController::DeleteGroup(const std::string& tournamentId,
                                                   const std::string& groupId) {
    auto res = groupDelegate->RemoveGroup(tournamentId, groupId);
    if (!res) {
        std::string err = res.error();
        int code = (err.find("doesn't exist") != std::string::npos) ? 404 : 422;
        return crow::response{code, err};
    }
    return crow::response{crow::NO_CONTENT};
}

// ============= ROUTE REGISTRATION (Registry/Macros) =============

REGISTER_ROUTE(GroupController, GetGroups,      "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup,       "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup,    "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup,    "/tournaments/<string>/groups/<string>", "PATCH"_method) // (stub mantenido)
REGISTER_ROUTE(GroupController, UpdateTeams,    "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeamToGroup, "/tournaments/<string>/groups/<string>/teams/<string>", "POST"_method)
REGISTER_ROUTE(GroupController, AddTeamToGroupFromBody, "/tournaments/<string>/groups/<string>", "POST"_method)

// === nuevas rutas sin afectar las anteriores ===
REGISTER_ROUTE(GroupController, RenameGroup,    "/tournaments/<string>/groups/<string>", "PUT"_method)
REGISTER_ROUTE(GroupController, DeleteGroup,    "/tournaments/<string>/groups/<string>", "DELETE"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
