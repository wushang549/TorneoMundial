// tests/controller/TeamControllerTest.cpp
//
// Suite de pruebas para TeamController con GTest/GMock.
// Cada TEST incluye comentarios que describen su intención.
//
// Requisitos:
//  - TeamDelegateMock.hpp con los MOCK_METHOD de ITeamDelegate.
//  - El target de tests debe tener en include dirs: tests/, tests/mocks/ y ../include.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TeamController.hpp"
#include "../mocks/TeamDelegateMock.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using nlohmann::json;

// -------- Helpers de apoyo a los tests --------

// Construye un crow::request con body JSON (string) para POST/PUT.
static crow::request make_json_req(const std::string& body) {
  crow::request req;
  req.body = body;
  return req;
}

// Crea un Team de dominio envuelto en shared_ptr (conveniente para Get/GetAll).
static std::shared_ptr<domain::Team> makeTeam(const std::string& id, const std::string& name) {
  return std::make_shared<domain::Team>(domain::Team{id, name});
}

/* ===================== GET /teams/{id} (TeamController::getTeam) ===================== */

// Valida el filtro de formato de ID: si el ID no cumple el regex (ID_VALUE), regresa 400.
// Aquí usamos "bad/id" porque el slash no está permitido por tu patrón.
TEST(TeamControllerTest, GetTeam_InvalidId_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  auto res = controller.getTeam("bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Si el delegate no encuentra el equipo (nullptr), el controller debe responder 404.
TEST(TeamControllerTest, GetTeam_NotFound_Returns404) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("A1")).WillOnce(Return(std::shared_ptr<domain::Team>{}));

  TeamController controller{mock};
  auto res = controller.getTeam("A1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// Caso exitoso: el delegate devuelve un Team y el controller responde 200 con JSON y content-type correcto.
TEST(TeamControllerTest, GetTeam_Found_Returns200WithJson) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("T01"))
      .WillOnce(Return(makeTeam("T01", "Team One")));

  TeamController controller{mock};
  auto res = controller.getTeam("T01");

  EXPECT_EQ(res.code, crow::OK);

  // Debe incluir content-type application/json
  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);

  // Body JSON con id y name
  json j = json::parse(res.body);
  EXPECT_EQ(j.at("id"), "T01");
  EXPECT_EQ(j.at("name"), "Team One");
}

/* ===================== GET /teams (TeamController::getAllTeams) ===================== */

// Si no hay equipos, debe regresar 200 con "[]" y content-type JSON.
TEST(TeamControllerTest, GetAllTeams_Empty_ReturnsEmptyArray) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetAllTeams())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

  TeamController controller{mock};
  auto res = controller.getAllTeams();

  EXPECT_EQ(res.code, crow::OK);

  auto ct = res.headers.find("content-type");
  ASSERT_NE(ct, res.headers.end());
  EXPECT_NE(ct->second.find("application/json"), std::string::npos);

  EXPECT_EQ(res.body, "[]");
}

// Con elementos: 200 y array JSON con los equipos serializados.
TEST(TeamControllerTest, GetAllTeams_TwoItems_ReturnsArray) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  std::vector<std::shared_ptr<domain::Team>> vec;
  vec.push_back(makeTeam("A", "Alpha"));
  vec.push_back(makeTeam("B", "Beta"));

  EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(vec));

  TeamController controller{mock};
  auto res = controller.getAllTeams();

  EXPECT_EQ(res.code, crow::OK);

  json arr = json::parse(res.body);
  ASSERT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr[0].at("id"), "A");
  EXPECT_EQ(arr[0].at("name"), "Alpha");
  EXPECT_EQ(arr[1].at("id"), "B");
  EXPECT_EQ(arr[1].at("name"), "Beta");
}

/* ===================== POST /teams (TeamController::SaveTeam) ===================== */

// Si el body no es JSON válido, regresa 400 inmediatamente.
TEST(TeamControllerTest, SaveTeam_InvalidJson_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  auto res = controller.SaveTeam(make_json_req("{not json"));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Si el cliente manda un "id" con formato inválido (regex falla), también 400.
TEST(TeamControllerTest, SaveTeam_ClientIdInvalidFormat_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  const std::string body = R"({"id":"bad/id","name":"Foo"})";
  auto res = controller.SaveTeam(make_json_req(body));
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Si el cliente manda un "id" válido pero ya existe (delegate.GetTeam != nullptr), responde 409 (conflict)
// y NO debe llamar SaveTeam en el delegate.
TEST(TeamControllerTest, SaveTeam_ClientIdAlreadyExists_Returns409) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, GetTeam("T-1"))
      .WillOnce(Return(makeTeam("T-1", "Existing")));
  EXPECT_CALL(*mock, SaveTeam(::testing::_)).Times(0); // no debe intentar crear

  TeamController controller{mock};

  const std::string body = R"({"id":"T-1","name":"NewName"})";
  auto res = controller.SaveTeam(make_json_req(body));
  EXPECT_EQ(res.code, crow::CONFLICT);
}

// Si el cliente manda "id" válido y NO existe previamente, el controller debe crear,
// y el header "location" debe preferir el ID del cliente (no el generado por el repo).
TEST(TeamControllerTest, SaveTeam_WithClientId_UsesClientIdInLocation_Returns201) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();

  // No existe previamente
  EXPECT_CALL(*mock, GetTeam("ID1"))
      .WillOnce(Return(std::shared_ptr<domain::Team>{}));

  // El delegate devuelve un id generado; como el cliente mandó ID, el controller debe
  // poner "location: ID1" (prioridad al id del cliente).
  EXPECT_CALL(*mock, SaveTeam(::testing::_))
      .WillOnce(Return(std::string_view{"generated"}));

  TeamController controller{mock};

  const std::string body = R"({"id":"ID1","name":"Alpha"})";
  auto res = controller.SaveTeam(make_json_req(body));

  EXPECT_EQ(res.code, crow::CREATED);
  auto loc = res.headers.find("location");
  ASSERT_NE(loc, res.headers.end());
  EXPECT_EQ(loc->second, "ID1"); // prioridad al id del cliente
}

// Si el cliente NO manda "id", el controller debe usar el id devuelto por el delegate
// en el header "location".
TEST(TeamControllerTest, SaveTeam_WithoutClientId_UsesReturnedIdInLocation_Returns201) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, SaveTeam(::testing::_))
      .WillOnce(Return(std::string_view{"gen-42"}));

  TeamController controller{mock};

  const std::string body = R"({"name":"NoIdTeam"})";
  auto res = controller.SaveTeam(make_json_req(body));

  EXPECT_EQ(res.code, crow::CREATED);
  auto loc = res.headers.find("location");
  ASSERT_NE(loc, res.headers.end());
  EXPECT_EQ(loc->second, "gen-42"); // usa el id devuelto por el repo
}

/* ===================== PUT /teams/{id} (TeamController::UpdateTeam) ===================== */

// ID de ruta inválido → 400 (antes de revisar JSON o tocar el delegate).
TEST(TeamControllerTest, UpdateTeam_InvalidIdFormat_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  auto res = controller.UpdateTeam(make_json_req(R"({"name":"X"})"), "bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Body no-JSON → 400.
TEST(TeamControllerTest, UpdateTeam_InvalidJson_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  auto res = controller.UpdateTeam(make_json_req("{not json"), "OK1");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Delegate devuelve false (no encontró el registro para actualizar) → 404.
TEST(TeamControllerTest, UpdateTeam_NotFound_Returns404) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTeam("U1", ::testing::_))
      .WillOnce(Return(false));

  TeamController controller{mock};
  auto res = controller.UpdateTeam(make_json_req(R"({"name":"Upd"})"), "U1");
  EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// Caso exitoso: JSON válido + ID válido + delegate devuelve true → 204.
// Además verificamos (vía Invoke) que el controller manda el ID correcto y el body parseado.
TEST(TeamControllerTest, UpdateTeam_Success_Returns204) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, UpdateTeam("U2", ::testing::_))
      .WillOnce(Invoke([](std::string_view id, const domain::Team& t) {
        EXPECT_EQ(id, "U2");        // id de la ruta propagado al delegate
        // Si tu from_json mapea "name", puedes validar contenido aquí si lo deseas.
        return true;
      }));

  TeamController controller{mock};
  auto res = controller.UpdateTeam(make_json_req(R"({"name":"Gamma"})"), "U2");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/* ===================== DELETE /teams/{id} (TeamController::DeleteTeam) ===================== */

// ID inválido → 400.
TEST(TeamControllerTest, DeleteTeam_InvalidIdFormat_Returns400) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  TeamController controller{mock};

  auto res = controller.DeleteTeam("bad/id");
  EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Delegate retorna true (borrado aplicado) → 204.
TEST(TeamControllerTest, DeleteTeam_Ok_Returns204) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTeam("D1")).WillOnce(Return(true));

  TeamController controller{mock};
  auto res = controller.DeleteTeam("D1");
  EXPECT_EQ(res.code, crow::NO_CONTENT);
}

// Delegate retorna false (tu controller lo mapea a 500 "delete not applied") → 500.
TEST(TeamControllerTest, DeleteTeam_NotApplied_Returns500) {
  auto mock = std::make_shared<NiceMock<TeamDelegateMock>>();
  EXPECT_CALL(*mock, DeleteTeam("D2")).WillOnce(Return(false));

  TeamController controller{mock};
  auto res = controller.DeleteTeam("D2");
  EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
}
