#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/TeamDelegate.hpp"
#include "mocks/TeamRepositoryMock.h"
#include "domain/Team.hpp"

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::An;
using namespace std::literals;

// Helper para crear objetos Team simulados
static std::shared_ptr<domain::Team> mkT(std::string id, std::string name){
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/* 
   Al método que procesa la creación de equipo, validar que el valor que 
   se le transfiera a TeamRepository es el esperado. 
   Simular una inserción válida y que la respuesta de esta función 
   sea el ID generado.
    */
TEST(TeamDelegateTest, SaveTeam_ReturnsIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_)).WillOnce(Return("id-1"sv));
  TeamDelegate sut{repo};
  domain::Team in{"","Nuevo"};
  EXPECT_EQ(sut.SaveTeam(in), "id-1");
}

/* 
   Al método que procesa la creación de equipo, validar que el valor que 
   se le transfiera a TeamRepository es el esperado.
   Simular una inserción fallida y que la respuesta de esta función 
   sea un error o mensaje usando std::expected.
    */
TEST(TeamDelegateTest, SaveTeam_Fails_ReturnsUnexpected) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_))
      .WillOnce(Return(std::unexpected<std::string>{"db error"})); 
  TeamDelegate sut{repo};
  domain::Team in{"", "New"};
  auto result = sut.SaveTeam(in);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "db error");
}

/* 
   Al método que procesa la búsqueda de un equipo por ID, validar que 
   el valor que se le transfiera a TeamRepository es el esperado.
   Simular el resultado con un objeto y validar el objeto.
    */
TEST(TeamDelegateTest, GetTeam_UsesReadByIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(mkT("X","XName")));
  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X");
  ASSERT_NE(t,nullptr);
  EXPECT_EQ(t->Name,"XName");
}

/* 
   Al método que procesa la búsqueda de un equipo por ID, validar que 
   el valor que se le transfiera a TeamRepository es el esperado.
   Simular el resultado nulo y validar nullptr.
    */
TEST(TeamDelegateTest, GetTeam_NotFound_ReturnsNull) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(nullptr));
  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X");
  EXPECT_EQ(t, nullptr);
}

/* 
   Al método que procesa la búsqueda de equipos. 
   Simular el resultado con una lista vacía de TeamRepository.
    */
TEST(TeamDelegateTest, GetAll_Empty){
  auto repo = std::make_shared<NiceMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  TeamDelegate sut{repo};
  EXPECT_TRUE(sut.GetAllTeams().empty());
}

/* 
   Al método que procesa la búsqueda de equipos. 
   Simular el resultado con una lista de objetos de TeamRepository.
    */
TEST(TeamDelegateTest, GetAll_WithItems_ReturnsList) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  std::vector<std::shared_ptr<domain::Team>> data{
      mkT("A", "Alpha"), mkT("B", "Beta")
  };
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(data));
  TeamDelegate sut{repo};
  auto res = sut.GetAllTeams();
  ASSERT_EQ(res.size(), 2u);
  EXPECT_EQ(res[0]->Name, "Alpha");
}

/* 
   Al método que procesa la actualización de un equipo, validar la 
   búsqueda de TeamRepository por ID, validar el valor transferido a Update.
   Simular resultado exitoso.
    */
TEST(TeamDelegateTest, Update_Found_CallsUpdateAndTrue){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("U2"sv)).WillOnce(Return(mkT("U2","Old")));
  EXPECT_CALL(*repo, Update(::testing::_)).WillOnce(Return("U2"sv));
  TeamDelegate sut{repo};
  domain::Team in{"U2","New"};
  EXPECT_TRUE(sut.UpdateTeam("U2", in));
}

/* 
   Al método que procesa la actualización de un equipo, validar la búsqueda 
   de TeamRepository por ID. Simular resultado de búsqueda no exitoso y 
   regresar error o mensaje usando std::expected.
    */
TEST(TeamDelegateTest, Update_NotFound_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById(An<std::string_view>())).WillOnce(Return(nullptr));
  EXPECT_CALL(*repo, Update(::testing::_)).Times(0);
  TeamDelegate sut{repo};
  domain::Team in{"U1","Name"};
  EXPECT_FALSE(sut.UpdateTeam("U1", in));
}

/* 
   Caso adicional, al método que procesa la eliminación de un equipo,
   validar que si no se encuentra el registro, el repositorio no se invoque
   para eliminar y se regrese false.
    */
TEST(TeamDelegateTest, Delete_NotFound_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("D1"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*repo, Delete(An<std::string_view>())).Times(0);
  TeamDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTeam("D1"));
}

/* 
   Caso adicional, al método que procesa la eliminación de un equipo,
   validar la búsqueda por ID y la eliminación posterior. 
   Simular resultado exitoso.
    */
TEST(TeamDelegateTest, Delete_Success_ReturnsTrue){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  TeamDelegate sut{repo};
  InSequence seq;
  EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(mkT("D2","ToDelete")));
  EXPECT_CALL(*repo, Delete("D2"sv)).Times(1);
  EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(nullptr));
  EXPECT_TRUE(sut.DeleteTeam("D2"));
}
