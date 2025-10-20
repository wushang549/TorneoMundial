#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/GroupController.hpp"
#include "mocks/GroupDelegateMock.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using nlohmann::json;
using namespace std::literals;

/* 
   Helpers
    */
static crow::request make_req(const std::string& body) { crow::request r; r.body = body; return r; }
static std::shared_ptr<domain::Group> mkGroup(std::string id, std::string name) {
    auto g = std::make_shared<domain::Group>();
    g->Id() = std::move(id);
    g->Name() = std::move(name);
    return g;
}

/* 
   Al método que procesa la creación de grupo para un torneo,
   validar la transformación de JSON al objeto de dominio Group y 
   validar que el valor que se le transfiera a GroupDelegate es el esperado.
   Validar que la respuesta de esta función sea HTTP 201
    */
TEST(GroupControllerTest, CreateGroup_Success_201) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, CreateGroup("T1"sv, _))
        .WillOnce(Invoke([](std::string_view tid, const domain::Group& g){
            EXPECT_EQ(tid, "T1");
            EXPECT_EQ(g.Name(), "Group A");
            EXPECT_EQ(g.Teams().size(), 1u);
            EXPECT_EQ(g.Teams()[0].Id, "team-01");
            return std::expected<std::string, std::string>{"G-123"};
        }));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"Group A","teams":[{"id":"team-01","name":"Team One"}]})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_THAT(res.body, ::testing::HasSubstr("G-123"));
}

/* 
   Al método que procesa la creación de grupo para un torneo,
   simular error de inserción en la base de datos y validar 
   la respuesta HTTP 409
    */
TEST(GroupControllerTest, CreateGroup_DBError_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, CreateGroup("T1"sv, _))
        .WillOnce(Return(std::unexpected("Failed to create group")));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"Group X"})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::CONFLICT);
}

/* 
   Al método que procesa la búsqueda de un grupo por ID y torneo,
   simular el resultado con un objeto y validar la respuesta HTTP 200
    */
TEST(GroupControllerTest, GetGroup_Found_200) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    auto grp = mkGroup("G1", "Alpha");
    grp->Teams().push_back(domain::Team{"T1","Team 1"});
    EXPECT_CALL(*mock, GetGroup("T1"sv, "G1"sv)).WillOnce(Return(grp));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "G1");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_THAT(res.body, ::testing::HasSubstr("\"Alpha\""));
    EXPECT_THAT(res.body, ::testing::HasSubstr("Team 1"));
}

/* 
   Al método que procesa la búsqueda de un grupo por ID y torneo,
   simular el resultado nulo y validar la respuesta HTTP 404
    */
TEST(GroupControllerTest, GetGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, GetGroup("T1"sv, "G2"sv)).WillOnce(Return(nullptr));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "G2");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* 
   Al método que procesa la actualización de un grupo,
   validar transformación JSON → Group y respuesta HTTP 204
    */
TEST(GroupControllerTest, RenameGroup_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateGroup("T1"sv, _))
        .WillOnce(Invoke([](std::string_view tid, const domain::Group& g){
            EXPECT_EQ(tid, "T1");
            EXPECT_EQ(g.Id(), "G1");
            EXPECT_EQ(g.Name(), "New Name");
            return std::expected<void, std::string>{};
        }));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"New Name"})");
    auto res = ctl.RenameGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/* 
   Al método que procesa la actualización de un grupo,
   simular ID no encontrado y validar respuesta HTTP 404
    */
TEST(GroupControllerTest, RenameGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateGroup("T1"sv, _))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"X"})");
    auto res = ctl.RenameGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* 
   Al método que procesa agregar un equipo a un grupo en un torneo,
   validar la transformación de JSON al objeto Team y respuesta HTTP 201
    */
TEST(GroupControllerTest, AddTeamToGroup_Success_201) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "T99"sv))
        .WillOnce(Return(std::expected<void, std::string>{}));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"T99","name":"Titans"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_THAT(res.body, ::testing::HasSubstr("success"));
}

/* 
   Al método que procesa agregar un equipo,
   simular que el equipo no exista y validar HTTP 404
    */
TEST(GroupControllerTest, AddTeamToGroup_TeamNotExist_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "TX"sv))
        .WillOnce(Return(std::unexpected("Team doesn't exist")));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"TX","name":"Ghost"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* 
   Al método que procesa agregar un equipo,
   simular que el grupo esté lleno y validar HTTP 409
    */
TEST(GroupControllerTest, AddTeamToGroup_GroupFull_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "TF"sv))
        .WillOnce(Return(std::unexpected("Group is full")));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"TF","name":"Overload"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::CONFLICT);
}
// ========== GetGroup: error del delegate (400) y not-found via mensaje (404) ==========
TEST(GroupControllerTest, GetGroup_DelegateError_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, GetGroup("T1"sv, "Gx"sv))
        .WillOnce(Return(std::unexpected("boom")));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "Gx");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
    EXPECT_THAT(res.body, ::testing::HasSubstr("boom"));
}

TEST(GroupControllerTest, GetGroup_NotFound_ByMessage_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, GetGroup("T1"sv, "Gz"sv))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "Gz");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// ========== GetGroups: success (200) y error (404) ==========
TEST(GroupControllerTest, GetGroups_Success_200) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    auto g1 = mkGroup("G1","A"); g1->Teams().push_back(domain::Team{"T1","One"});
    auto g2 = mkGroup("G2","B");
    EXPECT_CALL(*mock, GetGroups("T1"sv))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));

    GroupController ctl{mock};
    auto res = ctl.GetGroups("T1");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_THAT(res.body, ::testing::HasSubstr("\"G1\""));
    EXPECT_THAT(res.body, ::testing::HasSubstr("\"teams\""));
}

TEST(GroupControllerTest, GetGroups_DelegateError_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, GetGroups("T1"sv))
        .WillOnce(Return(std::unexpected("Tournament doesn't exist")));
    GroupController ctl{mock};
    auto res = ctl.GetGroups("T1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// ========== CreateGroup: bad JSON (400), missing name (400), invalid teams (400), not found (404) ==========
TEST(GroupControllerTest, CreateGroup_BadJson_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req("{bad json");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, CreateGroup_MissingName_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req(R"({"teams":[{"id":"t1","name":"n"}]})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, CreateGroup_TeamsNotArray_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req(R"({"name":"G","teams":123})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, CreateGroup_TeamMissingId_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req(R"({"name":"G","teams":[{"name":"N"}]})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, CreateGroup_TournamentNotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, CreateGroup("T1"sv, _))
        .WillOnce(Return(std::unexpected("Tournament doesn't exist")));
    GroupController ctl{mock};
    auto req = make_req(R"({"name":"G"})");
    auto res = ctl.CreateGroup(req, "T1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// ========== AddTeamToGroup: bad JSON (400), missing id (400), other error (400) ==========
TEST(GroupControllerTest, AddTeamToGroup_BadJson_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req("{bad json");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, AddTeamToGroup_MissingId_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto req = make_req(R"({"name":"X"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, AddTeamToGroup_OtherError_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "T2"sv))
        .WillOnce(Return(std::unexpected("validation failed")));
    GroupController ctl{mock};
    auto req = make_req(R"({"id":"T2","name":"N"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// ========== AddTeamToGroupById: success (204), not found (404), conflict (409), other (400) ==========
TEST(GroupControllerTest, AddTeamToGroupById_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "T9"sv))
        .WillOnce(Return(std::expected<void, std::string>{}));
    GroupController ctl{mock};
    auto res = ctl.AddTeamToGroupById("T1","G1","T9");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(GroupControllerTest, AddTeamToGroupById_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "Tx"sv))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));
    GroupController ctl{mock};
    auto res = ctl.AddTeamToGroupById("T1","G1","Tx");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(GroupControllerTest, AddTeamToGroupById_Conflict_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "Tz"sv))
        .WillOnce(Return(std::unexpected("capacity reached")));
    GroupController ctl{mock};
    auto res = ctl.AddTeamToGroupById("T1","G1","Tz");
    EXPECT_EQ(res.code, crow::CONFLICT);
}

TEST(GroupControllerTest, AddTeamToGroupById_Other_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "Ty"sv))
        .WillOnce(Return(std::unexpected("weird error")));
    GroupController ctl{mock};
    auto res = ctl.AddTeamToGroupById("T1","G1","Ty");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// ========== UpdateTeams: bad JSON (400), body not array (400), entry not object (400), missing id (400) ==========
TEST(GroupControllerTest, UpdateTeams_BadJson_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req("{bad json}"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, UpdateTeams_BodyNotArray_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"({"id":"x"})"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, UpdateTeams_EntryNotObject_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"([1,2,3])"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(GroupControllerTest, UpdateTeams_MissingId_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"([{"name":"N"}])"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// ========== UpdateTeams: success (204) y errores mapeados (404/409/400) ==========
TEST(GroupControllerTest, UpdateTeams_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateTeams("T1"sv, "G1"sv, ::testing::_))
        .WillOnce(Return(std::expected<void, std::string>{}));

    GroupController ctl{mock};
    // mixed ids: "Id" and "id"; name optional
    auto req = make_req(R"([{"Id":"A","name":"Alpha"},{"id":"B"}])");
    auto res = ctl.UpdateTeams(req, "T1", "G1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(GroupControllerTest, UpdateTeams_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateTeams("T1"sv, "G1"sv, ::testing::_))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"([{"id":"A"}])"), "T1", "G1");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(GroupControllerTest, UpdateTeams_Conflict_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateTeams("T1"sv, "G1"sv, ::testing::_))
        .WillOnce(Return(std::unexpected("capacity exceeded")));
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"([{"id":"A"}])"), "T1", "G1");
    EXPECT_EQ(res.code, crow::CONFLICT);
}

TEST(GroupControllerTest, UpdateTeams_OtherError_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateTeams("T1"sv, "G1"sv, ::testing::_))
        .WillOnce(Return(std::unexpected("validation failed")));
    GroupController ctl{mock};
    auto res = ctl.UpdateTeams(make_req(R"([{"id":"A"}])"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// ========== RenameGroup: bad JSON (400), missing name (400), conflict (409) ==========
TEST(RenameGroupTest, RenameGroup_BadJson_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.RenameGroup(make_req("{bad json}"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(RenameGroupTest, RenameGroup_MissingName_400) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    GroupController ctl{mock};
    auto res = ctl.RenameGroup(make_req(R"({"x":"y"})"), "T1", "G1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(RenameGroupTest, RenameGroup_Conflict_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, UpdateGroup("T1"sv, ::testing::_))
        .WillOnce(Return(std::unexpected("already exists")));
    GroupController ctl{mock};
    auto res = ctl.RenameGroup(make_req(R"({"name":"Dup"})"), "T1", "G1");
    EXPECT_EQ(res.code, crow::CONFLICT);
}

// ========== DeleteGroup: success (204), not found (404), conflict (409) ==========
TEST(GroupControllerTest, DeleteGroup_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, RemoveGroup("T1"sv, "G1"sv))
        .WillOnce(Return(std::expected<void, std::string>{}));
    GroupController ctl{mock};
    auto res = ctl.DeleteGroup("T1","G1");
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(GroupControllerTest, DeleteGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, RemoveGroup("T1"sv, "Gx"sv))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));
    GroupController ctl{mock};
    auto res = ctl.DeleteGroup("T1","Gx");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(GroupControllerTest, DeleteGroup_Conflict_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();
    EXPECT_CALL(*mock, RemoveGroup("T1"sv, "Gy"sv))
        .WillOnce(Return(std::unexpected("already assigned")));
    GroupController ctl{mock};
    auto res = ctl.DeleteGroup("T1","Gy");
    EXPECT_EQ(res.code, crow::CONFLICT);
}
