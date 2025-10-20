#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/TournamentDelegate.hpp"
#include "mocks/TournamentRepositoryMock.h"
#include "domain/Tournament.hpp"

using ::testing::StrictMock;
using ::testing::Return;
using ::testing::Invoke;

// Helper para crear objetos de prueba
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
   Al método que procesa la creación de torneo, validar que el valor 
   que se le transfiera a TournamentRepository es el esperado.
   Simular una inserción válida y que la respuesta de esta función 
   sea el ID generado.
    */
TEST(TournamentDelegateTest, Create_ReturnsRepoId){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_)).WillOnce(Return(std::string{"t-001"}));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{2,4,domain::TournamentType::ROUND_ROBIN};
  auto obj = std::make_shared<domain::Tournament>("Alpha", fmt);
  EXPECT_EQ(sut.CreateTournament(obj), "t-001");
}

/* 
   Al método que procesa la creación de torneo, validar que el valor 
   que se le transfiera a TournamentRepository es el esperado.
   Simular una inserción fallida y que la respuesta de esta función 
   sea un error o mensaje usando std::expected.
    */
TEST(TournamentDelegateTest, Create_Fails_ReturnsUnexpected){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_))
      .WillOnce(Return(std::unexpected<std::string>{"db error"}));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{2,4,domain::TournamentType::ROUND_ROBIN};
  auto obj = std::make_shared<domain::Tournament>("Fail", fmt);
  auto result = sut.CreateTournament(obj);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "db error");
}

/* 
   Al método que procesa la búsqueda de un torneo por ID, validar que 
   el valor que se le transfiera a TournamentRepository es el esperado.
   Simular el resultado con un objeto y validar valores del objeto.
    */
TEST(TournamentDelegateTest, ReadById_Found){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadById("x9")).WillOnce(Return(mkT("x9","X",1,8,domain::TournamentType::NFL)));
  TournamentDelegate sut{repo};
  auto t = sut.ReadById("x9");
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->Name(), "X");
}

/* 
   Al método que procesa la búsqueda de un torneo por ID, validar que 
   el valor que se le transfiera a TournamentRepository es el esperado.
   Simular el resultado nulo y validar nullptr.
    */
TEST(TournamentDelegateTest, ReadById_NotFound_ReturnsNull){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadById("NF1")).WillOnce(Return(nullptr));
  TournamentDelegate sut{repo};
  auto t = sut.ReadById("NF1");
  EXPECT_EQ(t, nullptr);
}

/* 
   Al método que procesa la búsqueda de torneos. 
   Simular el resultado con una lista vacía de TournamentRepository.
    */
TEST(TournamentDelegateTest, ReadAll_Empty){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  TournamentDelegate sut{repo};
  EXPECT_TRUE(sut.ReadAll().empty());
}

/* 
   Al método que procesa la búsqueda de torneos. 
   Simular el resultado con una lista de objetos de TournamentRepository.
    */
TEST(TournamentDelegateTest, ReadAll_WithItems_ReturnsList){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  std::vector<std::shared_ptr<domain::Tournament>> data{
      mkT("A","Alpha",2,4,domain::TournamentType::ROUND_ROBIN),
      mkT("B","Beta",1,8,domain::TournamentType::NFL)
  };
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(data));
  TournamentDelegate sut{repo};
  auto result = sut.ReadAll();
  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0]->Name(), "Alpha");
  EXPECT_EQ(result[1]->Name(), "Beta");
}

/* 
   Al método que procesa la actualización de un torneo, validar la 
   búsqueda de TournamentRepository por ID, validar el valor transferido 
   a Update. Simular resultado exitoso.
    */
TEST(TournamentDelegateTest, Update_Success_ReturnsTrue_OverwritesId){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Update(::testing::_))
    .WillOnce(Invoke([](const domain::Tournament& d){
      EXPECT_EQ(d.Id(), "U7");
      EXPECT_EQ(d.Name(), "Gamma");
      return std::string{"U7"};
    }));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{4,16,domain::TournamentType::ROUND_ROBIN};
  domain::Tournament in{"Gamma", fmt};
  in.Id() = "WRONG";
  EXPECT_TRUE(sut.UpdateTournament("U7", in));
}

/* 
   Al método que procesa la actualización de un torneo, validar la 
   búsqueda de TournamentRepository por ID. Simular resultado de 
   búsqueda no exitoso y regresar error o mensaje usando std::expected.
    */
TEST(TournamentDelegateTest, Update_NotFound_ReturnsFalse_NoException){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadById("U404")).WillOnce(Return(nullptr));
  EXPECT_CALL(*repo, Update(::testing::_)).Times(0);
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{1,4,domain::TournamentType::NFL};
  domain::Tournament in{"Gamma", fmt};
  EXPECT_FALSE(sut.UpdateTournament("U404", in));
}

/* 
   Caso adicional, al método que procesa la actualización de un torneo,
   simular que el repositorio lanza una excepción y validar que el 
   resultado sea false (manejo de errores).
    */
TEST(TournamentDelegateTest, Update_RepoThrows_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Update(::testing::_))
    .WillOnce(Invoke([](const domain::Tournament&){ throw std::runtime_error("db"); return std::string{}; }));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{1,4,domain::TournamentType::NFL};
  domain::Tournament in{"Omega", fmt};
  EXPECT_FALSE(sut.UpdateTournament("id", in));
}

/* 
   Caso adicional, al método que procesa la eliminación de un torneo, 
   validar que cuando el repositorio lanza una excepción, se regrese 
   false (manejo correcto de errores).
    */
TEST(TournamentDelegateTest, DeleteTournament_RepoThrows_ReturnsFalse) {
    auto repo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
    TournamentDelegate sut{repo};

    EXPECT_CALL(*repo, Delete("T2"))
        .WillOnce(Invoke([](std::string){ throw std::runtime_error("db"); }));

    EXPECT_FALSE(sut.DeleteTournament("T2"));
}

/* 
   Caso adicional, al método que procesa la eliminación de un torneo, 
   validar que el valor se transfiera correctamente al repositorio 
   y que el resultado sea true cuando no hay errores.
    */
TEST(TournamentDelegateTest, Delete_Success_ReturnsTrue){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D1")).Times(1);
  TournamentDelegate sut{repo};
  EXPECT_TRUE(sut.DeleteTournament("D1"));
}

/* 
   Caso adicional, al método que procesa la eliminación de un torneo,
   simular excepción del repositorio (por restricciones de FK o error 
   interno) y validar que el resultado sea false.
    */
TEST(TournamentDelegateTest, Delete_RepoThrows_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D2"))
    .WillOnce(Invoke([](std::string){ throw std::runtime_error("fk"); }));
  TournamentDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTournament("D2"));
}
