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

/*
   Helpers
    */
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

/*
   Al método que procesa la creación de torneo, validar la transformación
   de JSON al objeto de dominio Tournament y validar que el valor que se
   le transfiera a TournamentDelegate es el esperado.
   Simular error de inserción (nombre duplicado) y validar respuesta HTTP 409.
    */
TEST(TournamentControllerTest, Create_NameDuplicate_Returns409) {
    auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
    auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
    TournamentController ctl{del, grepo};

    auto t = std::make_shared<domain::Tournament>(
        domain::Tournament{"NFL 2025", domain::TournamentFormat{2,4,domain::TournamentType::NFL}});
    t->Id() = "T1";
    EXPECT_CALL(*del, ReadAll()).WillOnce(Return(std::vector{t}));

    crow::request req;
    req.body = R"({"name":"NFL 2025","format":{"numberOfGroups":2,"maxTeamsPerGroup":4,"type":"NFL"}})";
    auto res = ctl.CreateTournament(req);
    EXPECT_EQ(res.code, crow::CONFLICT);
}

/*
   Al método que procesa la búsqueda de un torneo por ID, validar que el
   valor que se le transfiera a TournamentDelegate es el esperado.
   Simular el resultado con un objeto y validar respuesta HTTP 200.
    */
TEST(TournamentControllerTest, ReadById_EmbedsGroups) {
    auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
    auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
    TournamentController ctl{del, grepo};

    auto t = std::make_shared<domain::Tournament>(
        domain::Tournament{"Copa", domain::TournamentFormat{1,3,domain::TournamentType::ROUND_ROBIN}});
    t->Id() = "T9";

    EXPECT_CALL(*del, ReadById("T9")).WillOnce(Return(t));

    auto g = std::make_shared<domain::Group>(domain::Group{"A", "G1"});
    g->TournamentId() = "T9";

    EXPECT_CALL(*grepo, FindByTournamentId("T9"sv))
        .WillOnce(Return(std::vector{ g }));

    auto res = ctl.ReadById("T9");
    EXPECT_EQ(res.code, crow::OK);
    EXPECT_EQ(res.get_header_value("content-type"), "application/json");
    EXPECT_THAT(std::string(res.body), ::testing::HasSubstr(R"("groups")"));
}

/*
   Al método que procesa la creación de torneo, validar que el cuerpo JSON
   sea correcto. Simular JSON inválido y validar respuesta HTTP 400.
    */
TEST(TournamentControllerTest, Create_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}};
  auto res = c.CreateTournament(req("{not json"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

/*
   Al método que procesa la creación de torneo, validar campos requeridos.
   Simular falta de atributo 'name' y validar respuesta HTTP 400.
    */
TEST(TournamentControllerTest, Create_MissingName_400) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}};
  auto res = c.CreateTournament(req(R"({"format":{"numberOfGroups":2,"maxTeamsPerGroup":4,"type":"ROUND_ROBIN"}})"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

/*
   Al método que procesa la creación de torneo, validar la transformación
   de JSON al objeto de dominio Tournament y el flujo exitoso.
   Validar que la respuesta sea HTTP 201 con location y cuerpo JSON.
    */
TEST(TournamentControllerTest, Create_Success_201_LocationBodyCT) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  EXPECT_CALL(*mock, CreateTournament(::testing::_)).WillOnce(Return(std::string{"t-001"}));

  TournamentController c{mock, {}};
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

/*
   Al método que procesa la búsqueda de torneos.
   Simular el resultado con una lista vacía y validar respuesta HTTP 200.
    */
TEST(TournamentControllerTest, ReadAll_Empty_200_Array) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  TournamentController c{mock, {}};
  auto res = c.ReadAll();
  EXPECT_EQ(res.code, crow::OK);
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);
  EXPECT_EQ(res.body, "[]");
}

/*
   Al método que procesa la búsqueda de torneos.
   Simular el resultado con una lista de objetos y validar respuesta HTTP 200.
    */
TEST(TournamentControllerTest, ReadAll_Two_200_Array) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  std::vector<std::shared_ptr<domain::Tournament>> v {
    mkT("a1","Alpha",2,4,domain::TournamentType::ROUND_ROBIN),
    mkT("b2","Beta",3,5,domain::TournamentType::NFL)
  };
  EXPECT_CALL(*mock, ReadAll()).WillOnce(Return(v));
  TournamentController c{mock, {}};
  auto res = c.ReadAll();
  EXPECT_EQ(res.code, crow::OK);
  json arr = json::parse(res.body);
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr[1].at("id"), "b2");
  EXPECT_EQ(arr[1].at("format").at("type"), "NFL");
}

/*
   Al método que procesa la búsqueda de un torneo por ID, validar que el
   valor que se le transfiera a TournamentDelegate es el esperado.
   Simular el resultado nulo y validar respuesta HTTP 404.
    */
TEST(TournamentControllerTest, ReadById_NotFound_404) {
  auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  EXPECT_CALL(*del, ReadById("X9")).WillOnce(Return(nullptr));
  TournamentController ctl{del, grepo};
  auto res = ctl.ReadById("X9");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/*
   Al método que procesa la actualización de un torneo, validar la
   transformación de JSON al objeto de dominio Tournament y que el
   valor transferido al TournamentDelegate sea el esperado.
   Simular JSON inválido y validar respuesta HTTP 400.
    */
TEST(TournamentControllerTest, Update_InvalidJson_400) {
  auto mock = std::make_shared<NiceMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}};
  auto res = c.UpdateTournament(req("{not json"), "ID");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

/*
   Al método que procesa la actualización de un torneo, validar la
   transformación de JSON al objeto de dominio Tournament y que el
   valor transferido a TournamentDelegate sea el esperado.
   Simular ID no encontrado y validar respuesta HTTP 404.
    */
TEST(TournamentControllerTest, Update_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("U7", ::testing::_)).WillOnce(Return(false));
  TournamentController c{mock, {}};
  auto res = c.UpdateTournament(req(R"({"name":"Gamma","format":{"numberOfGroups":1,"maxTeamsPerGroup":8,"type":"NFL"}})"), "U7");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/*
   Al método que procesa la actualización de un torneo, validar la
   transformación de JSON al objeto de dominio Tournament y que el
   valor transferido a TournamentDelegate sea el esperado.
   Simular resultado exitoso y validar respuesta HTTP 204.
    */
TEST(TournamentControllerTest, Update_Success_204) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("U7", ::testing::_)).WillOnce(Return(true));
  TournamentController c{mock, {}};
  auto res = c.UpdateTournament(req(R"({"name":"Gamma","format":{"numberOfGroups":1,"maxTeamsPerGroup":8,"type":"NFL"}})"), "U7");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/*
   Caso adicional, al método que procesa la eliminación de un torneo,
   validar flujo cuando el TournamentDelegate no encuentra el recurso.
   Simular ID inexistente y validar respuesta HTTP 404.
    */
TEST(TournamentControllerTest, Delete_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("D1")).WillOnce(Return(false));
  TournamentController c{mock, {}};
  auto res = c.DeleteTournament("D1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/*
   Caso adicional, al método que procesa la eliminación de un torneo,
   validar flujo exitoso. Simular eliminación exitosa y validar respuesta HTTP 204.
    */
TEST(TournamentControllerTest, Delete_Success_204) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("D2")).WillOnce(Return(true));
  TournamentController c{mock, {}};
  auto res = c.DeleteTournament("D2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}
// === Extra coverage for TournamentController error branches and esc(id) ===

TEST(TournamentControllerTest, Create_ReadAllUnexpected_500) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  TournamentController c{mock, {}};

  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::unexpected(std::string{"db list error"})));

  auto res = c.CreateTournament(req(R"({"name":"X","format":{"numberOfGroups":1,"maxTeamsPerGroup":4,"type":"NFL"}})"));
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("db list error"));
}

TEST(TournamentControllerTest, Create_DelegateCreateUnexpected_500) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  EXPECT_CALL(*mock, CreateTournament(::testing::_))
      .WillOnce(Return(std::unexpected(std::string{"insert fail"})));

  TournamentController c{mock, {}};
  auto res = c.CreateTournament(req(R"({"name":"X","format":{"numberOfGroups":1,"maxTeamsPerGroup":4,"type":"NFL"}})"));
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("insert fail"));
}

TEST(TournamentControllerTest, Create_Success_EscapesIdInBody) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  // id with quotes, backslash and newline to hit esc() branches
  std::string specialId = "t-\"q\\n";
  EXPECT_CALL(*mock, CreateTournament(::testing::_))
      .WillOnce(Return(specialId));

  TournamentController c{mock, {}};
  auto res = c.CreateTournament(req(R"({"name":"Esc","format":{"numberOfGroups":1,"maxTeamsPerGroup":4,"type":"NFL"}})"));
  EXPECT_EQ(res.code, crow::CREATED);
  // header carries the raw id
  EXPECT_EQ(res.get_header_value("location"), specialId);
  // body is valid JSON and preserves the same id value
  nlohmann::json j = nlohmann::json::parse(res.body);
  EXPECT_EQ(j.at("id").get<std::string>(), specialId);
}

TEST(TournamentControllerTest, ReadAll_Unexpected_500) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, ReadAll())
      .WillOnce(Return(std::unexpected(std::string{"boom"})));
  TournamentController c{mock, {}};
  auto res = c.ReadAll();
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("boom"));
}

TEST(TournamentControllerTest, ReadById_Unexpected_500) {
  auto del = std::make_shared<StrictMock<TournamentDelegateMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  EXPECT_CALL(*del, ReadById("E1"))
      .WillOnce(Return(std::unexpected(std::string{"err"})));
  TournamentController ctl{del, grepo};
  auto res = ctl.ReadById("E1");
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("err"));
}

TEST(TournamentControllerTest, Update_DelegateUnexpected_500) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTournament("U1", ::testing::_))
      .WillOnce(Return(std::unexpected(std::string{"upd fail"})));
  TournamentController c{mock, {}};
  auto res = c.UpdateTournament(
      req(R"({"name":"N","format":{"numberOfGroups":1,"maxTeamsPerGroup":4,"type":"NFL"}})"),
      "U1");
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("upd fail"));
}

TEST(TournamentControllerTest, Delete_DelegateUnexpected_500) {
  auto mock = std::make_shared<StrictMock<TournamentDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTournament("DX"))
      .WillOnce(Return(std::unexpected(std::string{"del fail"})));
  TournamentController c{mock, {}};
  auto res = c.DeleteTournament("DX");
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("del fail"));
}
