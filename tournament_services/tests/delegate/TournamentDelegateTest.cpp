// tests/delegate/TournamentDelegateTest.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/TournamentDelegate.hpp"
#include "mocks/TournamentRepositoryMock.h"
#include "domain/Tournament.hpp"

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::Invoke;

// Helper para crear torneos con Id asignado
static std::shared_ptr<domain::Tournament> makeTournament(
    const std::string& id, const std::string& name,
    int groups, int maxPerGroup, domain::TournamentType type)
{
  domain::TournamentFormat fmt{groups, maxPerGroup, type};
  auto t = std::make_shared<domain::Tournament>(name, fmt);
  t->Id() = id;
  return t;
}

/* ==================== CreateTournament ==================== */

TEST(TournamentDelegateTest, CreateTournament_ReturnsIdFromRepo) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();

  // El delegate pasa *tournament por valor (desreferenciado) al repo
  EXPECT_CALL(*repo, Create(::testing::_))
      .WillOnce(Return(std::string{"t-001"}));

  TournamentDelegate sut{repo};

  domain::TournamentFormat fmt{2, 4, domain::TournamentType::ROUND_ROBIN};
  auto obj = std::make_shared<domain::Tournament>("Alpha", fmt);

  auto id = sut.CreateTournament(obj);
  EXPECT_EQ(id, "t-001");
}

/* ==================== ReadAll ==================== */

TEST(TournamentDelegateTest, ReadAll_Empty) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(
      std::vector<std::shared_ptr<domain::Tournament>>{}));

  TournamentDelegate sut{repo};
  auto v = sut.ReadAll();
  EXPECT_TRUE(v.empty());
}

TEST(TournamentDelegateTest, ReadAll_TwoItems) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  std::vector<std::shared_ptr<domain::Tournament>> vec{
    makeTournament("a1","Alpha", 2, 4, domain::TournamentType::ROUND_ROBIN),
    makeTournament("b2","Beta",  3, 5, domain::TournamentType::NFL)
  };
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(vec));

  TournamentDelegate sut{repo};
  auto v = sut.ReadAll();
  ASSERT_EQ(v.size(), 2u);
  EXPECT_EQ(v[0]->Id(), "a1");
  EXPECT_EQ(v[0]->Name(), "Alpha");
  EXPECT_EQ(v[1]->Id(), "b2");
  EXPECT_EQ(v[1]->Format().Type(), domain::TournamentType::NFL);
}

/* ==================== ReadById ==================== */

TEST(TournamentDelegateTest, ReadById_NotFound) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadById("zzz"))
      .WillOnce(Return(std::shared_ptr<domain::Tournament>{}));

  TournamentDelegate sut{repo};
  auto t = sut.ReadById("zzz");
  EXPECT_EQ(t, nullptr);
}

TEST(TournamentDelegateTest, ReadById_Found) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  auto t = makeTournament("x9","X", 1, 8, domain::TournamentType::NFL);
  EXPECT_CALL(*repo, ReadById("x9")).WillOnce(Return(t));

  TournamentDelegate sut{repo};
  auto out = sut.ReadById("x9");
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(out->Name(), "X");
  EXPECT_EQ(out->Format().MaxTeamsPerGroup(), 8);
}

/* ==================== UpdateTournament ==================== */

TEST(TournamentDelegateTest, UpdateTournament_Success_ReturnsTrue_OverwritesId) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();

  // Verificamos que el delegate sobrescribe el Id con el de la ruta
  EXPECT_CALL(*repo, Update(::testing::_))
      .WillOnce(Invoke([](const domain::Tournament& d) {
        EXPECT_EQ(d.Id(), "U7");           // Id forzado por el delegate
        EXPECT_EQ(d.Name(), "Gamma");
        EXPECT_EQ(d.Format().NumberOfGroups(), 4);
        return std::string{"U7"};         // Update devuelve Id
      }));

  TournamentDelegate sut{repo};

  domain::TournamentFormat fmt{4, 16, domain::TournamentType::ROUND_ROBIN};
  domain::Tournament input{"Gamma", fmt};
  input.Id() = "WRONG-ID"; // El delegate debe ignorar esto y poner "U7"

  EXPECT_TRUE(sut.UpdateTournament("U7", input));
}

TEST(TournamentDelegateTest, UpdateTournament_RepoThrows_ReturnsFalse) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();

  EXPECT_CALL(*repo, Update(::testing::_))
      .WillOnce(Invoke([](const domain::Tournament&) -> std::string {
        throw std::runtime_error("db error");
      }));

  TournamentDelegate sut{repo};

  domain::TournamentFormat fmt{1, 4, domain::TournamentType::NFL};
  domain::Tournament input{"Omega", fmt};

  EXPECT_FALSE(sut.UpdateTournament("id", input));
}

/* ==================== DeleteTournament ==================== */

TEST(TournamentDelegateTest, DeleteTournament_Success_ReturnsTrue) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D1")).Times(1); // si no lanza, retorna true

  TournamentDelegate sut{repo};
  EXPECT_TRUE(sut.DeleteTournament("D1"));
}

TEST(TournamentDelegateTest, DeleteTournament_RepoThrows_ReturnsFalse) {
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D2"))
      .WillOnce(Invoke([](std::string) {
        throw std::runtime_error("fk constraint");
      }));

  TournamentDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTournament("D2"));
}
