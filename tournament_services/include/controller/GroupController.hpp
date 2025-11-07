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

public:
    explicit GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
        : groupDelegate(delegate) {}
    ~GroupController() = default;

    // GET /tournaments/{tid}/groups
    crow::response GetGroups(const std::string& tournamentId);

    // GET /tournaments/{tid}/groups/{gid}F
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);

    // POST /tournaments/{tid}/groups
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);

    // PATCH /tournaments/{tid}/groups/{gid}/teams  (batch)
    crow::response UpdateTeams(const crow::request& request,
                               const std::string& tournamentId,
                               const std::string& groupId);

    // POST /tournaments/{tid}/groups/{gid}/teams/{teamId}
    crow::response AddTeamToGroupById(const std::string& tournamentId,
                                      const std::string& groupId,
                                      const std::string& teamId);

    // POST /tournaments/{tid}/groups/{gid}  (equipo en body → 201)
    crow::response AddTeamToGroup(const crow::request& request,
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

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
