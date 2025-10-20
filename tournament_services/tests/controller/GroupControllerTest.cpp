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
