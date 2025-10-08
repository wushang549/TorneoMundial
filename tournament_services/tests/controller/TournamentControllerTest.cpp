#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TournamentController.hpp"
#include "../mocks/TournamentDelegateMock.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using nlohmann::json;

// Helper: crea un request con body JSON
static crow::request make_json_req(const std::string& jsonBody) {
  crow::request req;
  req.body = jsonBody;
  return req;
}

// Helpers para crear torneos de dominio
static std::shared_ptr<domain::Tournament> makeTournament(
    const std::string& id, const std::string& name,
    int groups, int maxPerGroup, domain::TournamentType type) {
  domain::TournamentFormat fmt{groups, maxPerGroup, type};
  auto t = std::make_shared<domain::Tournament>(name, fmt);
  t->Id() = id; // tu clase permite asignar Id() por referencia
  return t;
}

TEST(TournamentControllerTest, CreateTournament_ValidJson_Returns201AndBody) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, CreateTournament(::testing::_))
      .WillOnce(Return("abc-123"));

  TournamentController controller{mock};

  const std::string body = R"({
    "name":"T 10",
    "format":{"numberOfGroups":14,"maxTeamsPerGroup":4,"type":"NFL"}
  })";

  crow::request req = make_json_req(body);
  auto res = controller.CreateTournament(req);

  EXPECT_EQ(res.code, crow::CREATED);
  // Body: {"id":"abc-123"}
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "abc-123");

  // Header Location presente y correcto (si tu Crow expone headers así)
  auto it = res.headers.find("location");
  ASSERT_NE(it, res.headers.end());
  EXPECT_EQ(it->second, "abc-123");
}

TEST(TournamentControllerTest, CreateTournament_InvalidJson_Returns400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController controller{mock};

  crow::request req = make_json_req("{ invalid json ");
  auto res = controller.CreateTournament(req);

  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TournamentControllerTest, ReadAll_Empty_ReturnsEmptyArray) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

  TournamentController controller{mock};
  auto res = controller.ReadAll();

  EXPECT_EQ(res.code, crow::OK);
  EXPECT_EQ(res.body, "[]");
  auto it = res.headers.find("content-type");
  ASSERT_NE(it, res.headers.end());
  EXPECT_NE(it->second.find("application/json"), std::string::npos);
}

TEST(TournamentControllerTest, ReadAll_TwoItems_ReturnsJsonArray) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  std::vector<std::shared_ptr<domain::Tournament>> v;
  v.push_back(makeTournament("a1","Alpha", 2, 4, domain::TournamentType::ROUND_ROBIN));
  v.push_back(makeTournament("b2","Beta",  3, 5, domain::TournamentType::NFL));

  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(v));

  TournamentController controller{mock};
  auto res = controller.ReadAll();

  EXPECT_EQ(res.code, crow::OK);
  json arr = json::parse(res.body);
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr[0].at("id"),   "a1");
  EXPECT_EQ(arr[0].at("name"), "Alpha");
  EXPECT_EQ(arr[0]["format"]["numberOfGroups"], 2);
  EXPECT_EQ(arr[0]["format"]["type"], "ROUND_ROBIN");
  EXPECT_EQ(arr[1].at("id"),   "b2");
  EXPECT_EQ(arr[1]["format"]["type"], "NFL");
}

TEST(TournamentControllerTest, ReadById_Found_Returns200WithBody) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  auto t = makeTournament("z9","Zeta", 1, 8, domain::TournamentType::NFL);
  EXPECT_CALL(*mock, ReadById("z9"))
      .WillOnce(Return(t));

  TournamentController controller{mock};
  auto res = controller.ReadById("z9");

  EXPECT_EQ(res.code, crow::OK);
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "z9");
  EXPECT_EQ(j.at("name"), "Zeta");
  EXPECT_EQ(j["format"]["maxTeamsPerGroup"], 8);
}

TEST(TournamentControllerTest, ReadById_NotFound_Returns404) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadById("nope"))
      .WillOnce(Return(std::shared_ptr<domain::Tournament>{}));

  TournamentController controller{mock};
  auto res = controller.ReadById("nope");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TournamentControllerTest, UpdateTournament_ValidJson_PropagatesIdAndReturns204) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();

  // Verificamos que el controller asigne t.Id() = id antes de llamar al delegate
  EXPECT_CALL(*mock, UpdateTournament("u7", ::testing::_))
      .WillOnce(Invoke([](const std::string& id, const domain::Tournament& t) {
        EXPECT_EQ(id, "u7");
        EXPECT_EQ(t.Id(), "u7");             // el controller hizo t.Id() = id
        EXPECT_EQ(t.Name(), "Gamma");
        EXPECT_EQ(t.Format().NumberOfGroups(), 4);
        EXPECT_EQ(t.Format().MaxTeamsPerGroup(), 16);
        EXPECT_EQ(t.Format().Type(), domain::TournamentType::ROUND_ROBIN);
        return true;
      }));

  TournamentController controller{mock};
  const std::string body = R"({
    "name":"Gamma",
    "format":{"numberOfGroups":4,"maxTeamsPerGroup":16,"type":"ROUND_ROBIN"}
  })";
  auto res = controller.UpdateTournament(make_json_req(body), "u7");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(TournamentControllerTest, UpdateTournament_NotFound_Returns404) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("xx", ::testing::_))
      .WillOnce(Return(false));

  TournamentController controller{mock};
  const std::string body = R"({"name":"X","format":{"numberOfGroups":1,"maxTeamsPerGroup":2,"type":"NFL"}})";
  auto res = controller.UpdateTournament(make_json_req(body), "xx");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TournamentControllerTest, UpdateTournament_InvalidJson_Returns400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController controller{mock};

  auto res = controller.UpdateTournament(make_json_req("{not json"), "id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TournamentControllerTest, DeleteTournament_Ok_Returns204) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("d1"))
      .WillOnce(Return(true));

  TournamentController controller{mock};
  auto res = controller.DeleteTournament("d1");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(TournamentControllerTest, DeleteTournament_NotFound_Returns404) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("missing"))
      .WillOnce(Return(false));

  TournamentController controller{mock};
  auto res = controller.DeleteTournament("missing");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}
