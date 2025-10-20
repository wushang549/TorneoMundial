#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TeamController.hpp"
#include "mocks/TeamDelegateMock.hpp"

using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::An;
using nlohmann::json;
using namespace std::literals;

// Helpers
static crow::request make_req(const std::string& body) { crow::request r; r.body = body; return r; }
static std::shared_ptr<domain::Team> mkTeam(std::string id, std::string name) {
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/* 
   Al método que procesa la creación de equipo, validar la transformación 
   de JSON al objeto de dominio Team y validar que el valor que se le 
   transfiera a TeamDelegate es el esperado. 
   Simular error de inserción en la base de datos y validar la respuesta HTTP 409.
    */
TEST(TeamControllerTest, SaveTeam_NameDuplicate_Returns409) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController ctl{mock};

  // Simula que el repositorio ya contiene un equipo con el mismo nombre
  EXPECT_CALL(*mock, GetAllTeams())
      .WillOnce(Return(std::vector{
         std::make_shared<domain::Team>(domain::Team{"A1","Eagles"})
      }));

  crow::request req;
  req.body = R"({"name":"Eagles"})";
  auto res = ctl.SaveTeam(req);
  EXPECT_EQ(res.code, crow::CONFLICT);
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("already exists"));
}

/* 
   Caso adicional: Validar ID del cliente inválido al crear un equipo.
   Simular ID no conforme al formato y validar respuesta HTTP 400.
    */
TEST(TeamControllerTest, SaveTeam_ClientIdInvalid_Returns400) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController ctl{mock};

  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

  crow::request req;
  req.body = R"({"id":"bad id con espacios", "name":"Jets"})";
  auto res = ctl.SaveTeam(req);
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

/* 
   Al método que procesa la creación de equipo, validar la transformación 
   de JSON al objeto de dominio Team y que el valor se transfiera 
   correctamente al TeamDelegate. Validar respuesta HTTP 201.
    */
TEST(TeamControllerTest, SaveTeam_Success_NoClientId_SetsLocationAndJson) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController ctl{mock};

  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  EXPECT_CALL(*mock, SaveTeam(::testing::_)).WillOnce(Return(std::string_view{"GEN-123"}));

  crow::request req;
  req.body = R"({"name":"Bears"})";
  auto res = ctl.SaveTeam(req);
  EXPECT_EQ(res.code, crow::CREATED);
  EXPECT_EQ(res.get_header_value("location"), "GEN-123");
  EXPECT_EQ(res.get_header_value("content-type"), "application/json");
  EXPECT_THAT(std::string(res.body), ::testing::HasSubstr(R"("id":"GEN-123")"));
}

/* 
   Al método que procesa la actualización de un equipo, validar la 
   transformación de JSON al objeto de dominio Team y validar que el 
   valor se transfiera a TeamDelegate. Simular ID no encontrado y 
   validar respuesta HTTP 404.
    */
TEST(TeamControllerTest, UpdateTeam_NotFound_Returns404) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController ctl{mock};

  EXPECT_CALL(*mock, UpdateTeam("T1", ::testing::_)).WillOnce(Return(false));

  crow::request req; req.body = R"({"name":"NewName"})";
  auto res = ctl.UpdateTeam(req, "T1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* 
   Caso adicional: Al método que procesa la eliminación de un equipo,
   simular una excepción interna y validar respuesta HTTP 500.
    */
TEST(TeamControllerTest, DeleteTeam_Exception_Returns500) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  TeamController ctl{mock};

  EXPECT_CALL(*mock, DeleteTeam("T1"sv))
       .WillOnce(::testing::Invoke([](std::string_view) -> bool {
           throw std::runtime_error("db");
       }));
  auto res = ctl.DeleteTeam("T1");
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
}

/* 
   Al método que procesa la búsqueda de un equipo por ID, validar que 
   el valor que se le transfiera a TeamDelegate es el esperado. 
   Simular resultado nulo y validar respuesta HTTP 404.
    */
TEST(TeamControllerTest, GetTeam_NotFound_404) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("A1"sv)).WillOnce(Return(nullptr));
  TeamController c{mock};
  auto res = c.getTeam("A1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/* 
   Al método que procesa la búsqueda de un equipo por ID, validar que 
   el valor que se le transfiera a TeamDelegate es el esperado. 
   Simular resultado válido y validar respuesta HTTP 200.
    */
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

/* 
   Al método que procesa la búsqueda de equipos. 
   Simular el resultado con una lista vacía y validar respuesta HTTP 200.
    */
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

/* 
   Al método que procesa la búsqueda de equipos. 
   Simular el resultado con una lista de objetos y validar respuesta HTTP 200.
    */
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

/* 
   Al método que procesa la actualización de un equipo, validar la 
   transformación de JSON al objeto de dominio Team y validar que el 
   valor se le transfiera correctamente al TeamDelegate. 
   Validar respuesta HTTP 204 (actualización exitosa).
    */
TEST(TeamControllerTest, UpdateTeam_Ok_204) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTeam("U2"sv, ::testing::_)).WillOnce(Return(true));
  TeamController c{mock};
  auto res = c.UpdateTeam(make_req(R"({"name":"Gamma"})"), "U2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/* 
   Caso adicional, al método que procesa la eliminación de un equipo, 
   validar que se elimine correctamente y se retorne HTTP 204.
    */
TEST(TeamControllerTest, DeleteTeam_Ok_204) {
  auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTeam("D2"sv)).WillOnce(Return(true));
  TeamController c{mock};
  auto res = c.DeleteTeam("D2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}
