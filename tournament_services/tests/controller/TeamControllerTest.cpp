#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TeamController.hpp"
#include "mocks/TeamDelegateMock.hpp"

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::An;
using nlohmann::json;
using namespace std::literals;

static crow::request make_req(const std::string& body) { crow::request r; r.body = body; return r; }
static std::shared_ptr<domain::Team> mkTeam(std::string id, std::string name) {
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/* ============ GET /teams/{id} ============ */

TEST(TeamControllerTest, GetTeam_InvalidId_400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.getTeam("bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, GetTeam_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("A1"sv)).WillOnce(Return(nullptr));
  TeamController c{mock};
  auto res = c.getTeam("A1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TeamControllerTest, GetTeam_Ok_200_WithJsonAndCT) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("T01"sv)).WillOnce(Return(mkTeam("T01","One")));
  TeamController c{mock};
  auto res = c.getTeam("T01");
  EXPECT_EQ(res.code, crow::OK);
  auto it = res.headers.find("content-type");
  ASSERT_NE(it, res.headers.end());
  EXPECT_NE(it->second.find("application/json"), std::string::npos);
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "T01");
  EXPECT_EQ(j.at("name"), "One");
}

/* ============ GET /teams ============ */

TEST(TeamControllerTest, GetAllTeams_Empty_200_Array) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  TeamController c{mock};
  auto res = c.getAllTeams();
  EXPECT_EQ(res.code, crow::OK);
  auto it = res.headers.find("content-type");
  ASSERT_NE(it, res.headers.end());
  EXPECT_NE(it->second.find("application/json"), std::string::npos);
  EXPECT_EQ(res.body, "[]");
}

TEST(TeamControllerTest, GetAllTeams_TwoItems_200_Array) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  std::vector<std::shared_ptr<domain::Team>> v{ mkTeam("A","Alpha"), mkTeam("B","Beta") };
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(v));
  TeamController c{mock};
  auto res = c.getAllTeams();
  EXPECT_EQ(res.code, crow::OK);
  json arr = json::parse(res.body);
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr[0].at("id"), "A");
  EXPECT_EQ(arr[0].at("name"), "Alpha");
}

/* ============ POST /teams (SaveTeam) ============ */

TEST(TeamControllerTest, SaveTeam_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.SaveTeam(make_req("{not json"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, SaveTeam_MissingName_400) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"id":"ID1"})"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, SaveTeam_DuplicateName_409) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{ mkTeam("X","Alpha") }));
  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"name":"Alpha"})"));
  EXPECT_EQ(res.code, crow::CONFLICT);
}

TEST(TeamControllerTest, SaveTeam_ClientIdInvalid_400) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"id":"bad/id","name":"Z"})"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, SaveTeam_ClientIdExists_409) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  EXPECT_CALL(*mock, GetTeam("ID1"sv)).WillOnce(Return(mkTeam("ID1","Old")));
  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"id":"ID1","name":"Z"})"));
  EXPECT_EQ(res.code, crow::CONFLICT);
}

TEST(TeamControllerTest, SaveTeam_WithClientId_201_Location_AndBodyCT) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  // no dup name
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  // id libre
  EXPECT_CALL(*mock, GetTeam("ID1"sv)).WillOnce(Return(nullptr));
  // create devuelve algo (da igual, el controller preferirá el del cliente)
  EXPECT_CALL(*mock, SaveTeam(::testing::_)).WillOnce(Return("generated"sv));

  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"id":"ID1","name":"New"})"));
  EXPECT_EQ(res.code, crow::CREATED);
  auto loc = res.headers.find("location");
  ASSERT_NE(loc, res.headers.end());
  EXPECT_EQ(loc->second, "ID1");
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "ID1");
}

TEST(TeamControllerTest, SaveTeam_WithoutClientId_201_UsesReturnedId) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  EXPECT_CALL(*mock, SaveTeam(::testing::_)).WillOnce(Return("gen-42"sv));
  TeamController c{mock};
  auto res = c.SaveTeam(make_req(R"({"name":"SoloName"})"));
  EXPECT_EQ(res.code, crow::CREATED);
  auto loc = res.headers.find("location");
  ASSERT_NE(loc, res.headers.end());
  EXPECT_EQ(loc->second, "gen-42");
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "gen-42");
}

/* ============ PUT /teams/{id} (UpdateTeam) ============ */

TEST(TeamControllerTest, UpdateTeam_InvalidId_400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.UpdateTeam(make_req(R"({"name":"X"})"), "bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, UpdateTeam_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.UpdateTeam(make_req("{not json"), "ID1");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, UpdateTeam_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTeam("U1"sv, ::testing::_)).WillOnce(Return(false));
  TeamController c{mock};
  auto res = c.UpdateTeam(make_req(R"({"name":"Upd"})"), "U1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TeamControllerTest, UpdateTeam_Ok_204) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTeam("U2"sv, ::testing::_)).WillOnce(Return(true));
  TeamController c{mock};
  auto res = c.UpdateTeam(make_req(R"({"name":"Gamma"})"), "U2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/* ============ DELETE /teams/{id} ============ */

TEST(TeamControllerTest, DeleteTeam_InvalidId_400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController c{mock};
  auto res = c.DeleteTeam("bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TeamControllerTest, DeleteTeam_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTeam("D1"sv)).WillOnce(Return(false));
  TeamController c{mock};
  auto res = c.DeleteTeam("D1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TeamControllerTest, DeleteTeam_Ok_204) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTeam("D2"sv)).WillOnce(Return(true));
  TeamController c{mock};
  auto res = c.DeleteTeam("D2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}
