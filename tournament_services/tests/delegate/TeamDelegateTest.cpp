// tests/delegate/TeamDelegateTest.cpp
//
// Suite de pruebas unitaria para TeamDelegate.
// Requiere:
//  - MockTeamRepository en tests/mocks/TeamRepositoryMock.h
//  - TeamDelegate.hpp en tournament_services/include/delegate/TeamDelegate.hpp
//  - Team.hpp en tournament_common/include/domain/Team.hpp
//
// Asegúrate en CMake de agregar:
//   ../src/delegate/TeamDelegate.cpp
// y de incluir los include dirs de tests/, tests/mocks/ y tournament_common/include.

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

using namespace std::literals; // para "ID"sv

// Helper para crear equipos
static std::shared_ptr<domain::Team> makeTeam(std::string id, std::string name) {
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/* ==================== GetAllTeams ==================== */

TEST(TeamDelegateTest, GetAllTeams_Empty) {
  auto repo = std::make_shared<NiceMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

  TeamDelegate sut{repo};
  auto v = sut.GetAllTeams();
  EXPECT_TRUE(v.empty());
}

TEST(TeamDelegateTest, GetAllTeams_TwoItems) {
  auto repo = std::make_shared<NiceMock<MockTeamRepository>>();
  std::vector<std::shared_ptr<domain::Team>> vec{
      makeTeam("A", "Alpha"),
      makeTeam("B", "Beta")
  };
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(vec));

  TeamDelegate sut{repo};
  auto v = sut.GetAllTeams();
  ASSERT_EQ(v.size(), 2u);
  EXPECT_EQ(v[0]->Id, "A");
  EXPECT_EQ(v[1]->Name, "Beta");
}

/* ==================== GetTeam(id) ==================== */

TEST(TeamDelegateTest, GetTeam_UsesReadByIdView) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv))
      .WillOnce(Return(makeTeam("X","XName")));

  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X"); // std::string_view
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->Id, "X");
  EXPECT_EQ(t->Name, "XName");
}

/* ==================== SaveTeam(team) ==================== */

TEST(TeamDelegateTest, SaveTeam_ReturnsIdView) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_))
      .WillOnce(Return("id-1"sv)); // string_view con lifetime estático (literal)

  TeamDelegate sut{repo};
  domain::Team input{"", "Nuevo"};
  auto id = sut.SaveTeam(input);
  EXPECT_EQ(id, "id-1");
}

/* ==================== UpdateTeam(id, team) ==================== */

TEST(TeamDelegateTest, UpdateTeam_NotFound_ReturnsFalse) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById(An<std::string_view>()))
      .WillOnce(Return(std::shared_ptr<domain::Team>{})); // nullptr
  // No debe llamar Update si no existe
  EXPECT_CALL(*repo, Update(::testing::_)).Times(0);

  TeamDelegate sut{repo};
  domain::Team t{"U1","Name"};
  EXPECT_FALSE(sut.UpdateTeam("U1", t));
}

TEST(TeamDelegateTest, UpdateTeam_Found_CallsUpdateAndReturnsTrue) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("U2"sv))
      .WillOnce(Return(makeTeam("U2","Old")));

  // Update debe devolver string_view (Id)
  EXPECT_CALL(*repo, Update(::testing::_))
      .WillOnce(Return("U2"sv));

  TeamDelegate sut{repo};
  domain::Team t{"U2","New"};
  EXPECT_TRUE(sut.UpdateTeam("U2", t));
}

/* ==================== DeleteTeam(id) ==================== */

TEST(TeamDelegateTest, DeleteTeam_NotFound_ReturnsFalse_NoDeleteCall) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("D1"sv))
      .WillOnce(Return(std::shared_ptr<domain::Team>{})); // no existe
  EXPECT_CALL(*repo, Delete(An<std::string_view>())).Times(0);

  TeamDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTeam("D1"));
}

TEST(TeamDelegateTest, DeleteTeam_Success_ReturnsTrue_AfterPostCheck) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  TeamDelegate sut{repo};

  InSequence seq; // fuerza el orden: pre-check -> delete -> post-check

  // pre-check: existe
  EXPECT_CALL(*repo, ReadById("D2"sv))
      .WillOnce(Return(makeTeam("D2","ToDelete")));

  // delete: se invoca exactamente una vez con el mismo id
  EXPECT_CALL(*repo, Delete("D2"sv)).Times(1);

  // post-check: ya no existe
  EXPECT_CALL(*repo, ReadById("D2"sv))
      .WillOnce(Return(std::shared_ptr<domain::Team>{}));

  EXPECT_TRUE(sut.DeleteTeam("D2"));
}
