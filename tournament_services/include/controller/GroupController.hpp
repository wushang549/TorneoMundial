#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#include <vector>
#include <string>
#include <memory>
#include <crow.h>

#include "configuration/RouteDefinition.hpp"
#include "domain/Group.hpp"
#include "delegate/GroupDelegate.hpp"


class GroupController
{
private:
    std::shared_ptr<GroupDelegate> groupDelegate;
public:
    GroupController(const std::shared_ptr<GroupDelegate>& delegate);
    ~GroupController();
    crow::response GetGroups(const std::string& tournamentId);
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response CreateGroup(const crow::request& request);
    crow::response UpdateGroup(const crow::request& request);
};

GroupController::GroupController(const std::shared_ptr<GroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId){
    return crow::response{crow::NOT_IMPLEMENTED};
}
crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    return crow::response{crow::NOT_IMPLEMENTED};
}
crow::response GroupController::CreateGroup(const crow::request& request){
    return crow::response{crow::NOT_IMPLEMENTED};
}
crow::response GroupController::UpdateGroup(const crow::request& request){
    return crow::response{crow::NOT_IMPLEMENTED};
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
