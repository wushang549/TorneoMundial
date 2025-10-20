#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TournamentController.hpp"
#include "mocks/TournamentDelegateMock.hpp"
#include "mocks/GroupRepositoryMock.hpp"
using namespace std::literals;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using nlohmann::json;

/* Helpers */
static crow::request req(const std::string& b){ crow::request r; r.body=b; return r; }
static std::shared_ptr<domain::Tournament> mkT(
  const std::string& id, const std::string& name,
  int groups, int maxPerGroup, domain::TournamentType type)
{
  domain::TournamentFormat fmt{groups, maxPerGroup, type};
  auto p = std::make_shared<domain::Tournament>(name, fmt);
  p->Id() = id;
  return p;
}
// CreateTournament_NameDuplicate_Returns409
TEST(TournamentControllerTest, Create_NameDuplicate_Returns409) {
    auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
    auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
    TournamentController ctl{del, grepo};

    auto t = std::make_shared<domain::Tournament>(
        domain::Tournament{"NFL 2025", domain::TournamentFormat{2,4,domain::TournamentType::NFL}});
    t->Id() = "T1";
    EXPECT_CALL(*del, ReadAll()).WillOnce(Return(std::vector{t}));

    crow::request req; req.body = R"({"name":"NFL 2025","format":{"numberOfGroups":2,"maxTeamsPerGroup":4,"type":"NFL"}})";
    auto res = ctl.CreateTournament(req);
    EXPECT_EQ(res.code, crow::CONFLICT);
}

// ReadById_EmbedsGroups
TEST(TournamentControllerTest, ReadById_EmbedsGroups) {
    auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
    auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
    TournamentController ctl{del, grepo};

    auto t = std::make_shared<domain::Tournament>(
        domain::Tournament{"Copa", domain::TournamentFormat{1,3,domain::TournamentType::ROUND_ROBIN}});
    t->Id() = "T9";

    EXPECT_CALL(*del, ReadById("T9")).WillOnce(Return(t));

    auto g = std::make_shared<domain::Group>(domain::Group{"A", "G1"}); // ctor: name, id
    g->TournamentId() = "T9";                                           // set tournamentId

    EXPECT_CALL(*grepo, FindByTournamentId("T9"sv))
        .WillOnce(Return(std::vector{ g }));

    auto res = ctl.ReadById("T9");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");
    EXPECT_THAT(std::string(res.body), ::testing::HasSubstr(R"("groups")"));
}

/* ============ POST /tournaments (CreateTournament) ============ */

TEST(TournamentControllerTest, Create_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}}; // <-- segundo arg: groupRepo nulo
  auto res = c.CreateTournament(req("{not json"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TournamentControllerTest, Create_MissingName_400) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.CreateTournament(req(R"({"format":{"numberOfGroups":2,"maxTeamsPerGroup":4,"type":"ROUND_ROBIN"}})"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TournamentControllerTest, Create_DuplicateName_409) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{
        mkT("x","Alpha",2,4,domain::TournamentType::ROUND_ROBIN)
      }));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.CreateTournament(req(R"({"name":"Alpha","format":{"numberOfGroups":2,"maxTeamsPerGroup":4,"type":"ROUND_ROBIN"}})"));
  EXPECT_EQ(res.code, crow::CONFLICT);
}

TEST(TournamentControllerTest, Create_Success_201_LocationBodyCT) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  EXPECT_CALL(*mock, CreateTournament(::testing::_)).WillOnce(Return(std::string{"t-001"}));

  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.CreateTournament(req(R"({"name":"New","format":{"numberOfGroups":1,"maxTeamsPerGroup":8,"type":"NFL"}})"));
  EXPECT_EQ(res.code, crow::CREATED);
  auto loc = res.headers.find("location");
  ASSERT_NE(loc, res.headers.end());
  EXPECT_EQ(loc->second, "t-001");
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "t-001");
}

/* ============ GET /tournaments ============ */

TEST(TournamentControllerTest, ReadAll_Empty_200_Array) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.ReadAll();
  EXPECT_EQ(res.code, crow::OK);
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);
  EXPECT_EQ(res.body, "[]");
}

TEST(TournamentControllerTest, ReadAll_Two_200_Array) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  std::vector<std::shared_ptr<domain::Tournament>> v {
    mkT("a1","Alpha",2,4,domain::TournamentType::ROUND_ROBIN),
    mkT("b2","Beta",3,5,domain::TournamentType::NFL)
  };
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(v));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.ReadAll();
  EXPECT_EQ(res.code, crow::OK);
  json arr = json::parse(res.body);
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr[1].at("id"), "b2");
  EXPECT_EQ(arr[1].at("format").at("type"), "NFL");
}

/* ============ GET /tournaments/{id} ============ */
/* Nota: ReadById también embebe groups via groupRepository->FindByTournamentId(id).
   Para testear ReadById, inyecta un mock de IGroupRepository en lugar de {}. */

/* ============ PUT /tournaments/{id} ============ */

TEST(TournamentControllerTest, Update_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.UpdateTournament(req("{not json"), "ID");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(TournamentControllerTest, Update_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("U7", ::testing::_)).WillOnce(Return(false));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.UpdateTournament(req(R"({"name":"Gamma","format":{"numberOfGroups":1,"maxTeamsPerGroup":8,"type":"NFL"}})"), "U7");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TournamentControllerTest, Update_Success_204) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("U7", ::testing::_)).WillOnce(Return(true));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.UpdateTournament(req(R"({"name":"Gamma","format":{"numberOfGroups":1,"maxTeamsPerGroup":8,"type":"NFL"}})"), "U7");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/* ============ DELETE /tournaments/{id} ============ */

TEST(TournamentControllerTest, Delete_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("D1")).WillOnce(Return(false));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.DeleteTournament("D1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(TournamentControllerTest, Delete_Success_204) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("D2")).WillOnce(Return(true));
  TournamentController c{mock, {}}; // <-- segundo arg
  auto res = c.DeleteTournament("D2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}
