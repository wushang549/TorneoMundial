#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/GroupDelegate.hpp"
#include "mocks/GroupRepositoryMock.hpp"
#include "mocks/TournamentRepositoryMock.h"
#include "mocks/TeamRepositoryMock.h"   // usa .h si así lo tienes en tu repo

#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using namespace std::literals;

// ===== Helpers de dominio =====
static std::shared_ptr<domain::Tournament> mkT(
  std::string id, std::string name, int groups, int maxPerGroup, domain::TournamentType type)
{
  domain::TournamentFormat fmt{groups, maxPerGroup, type};
  auto t = std::make_shared<domain::Tournament>(name, fmt);
  t->Id() = std::move(id);
  return t;
}

static std::shared_ptr<domain::Group> mkG(std::string id, std::string name, std::string tid) {
  domain::Group g;
  g.Id() = std::move(id);
  g.Name() = std::move(name);
  g.TournamentId() = std::move(tid);
  return std::make_shared<domain::Group>(g);
}

static std::shared_ptr<domain::Team> mkTeam(std::string id, std::string name) {
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/* ================== CreateGroup ================== */

TEST(GroupDelegateTest, CreateGroup_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g; g.Name() = "G1";
  auto r = sut.CreateGroup("T0", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

TEST(GroupDelegateTest, CreateGroup_TeamMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::ROUND_ROBIN)));
  EXPECT_CALL(*teamr, ReadById("X1"sv)).WillOnce(Return(nullptr)); // string_view

  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group g;
  g.Name() = "G1";
  g.TournamentId() = "IGNORED";
  g.Teams().push_back(domain::Team{"X1","XName"});

  auto r = sut.CreateGroup("T1", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Team doesn't exist");
}

TEST(GroupDelegateTest, CreateGroup_Success_ReturnsId) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T2"))
      .WillOnce(Return(mkT("T2","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, Create(::testing::_))
      .WillOnce(Return(std::string{"G-001"}));

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g; g.Name() = "G1";
  auto r = sut.CreateGroup("T2", g);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value(), "G-001");
}

/* ================== GetGroups / GetGroup ================== */

TEST(GroupDelegateTest, GetGroups_RepoThrows_ReturnsError) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*grepo, FindByTournamentId("T1"sv))
      .WillOnce(Invoke([](const std::string_view&)->std::vector<std::shared_ptr<domain::Group>>{
        throw std::runtime_error("db");
      }));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroups("T1");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Error when reading to DB");
}

TEST(GroupDelegateTest, GetGroup_Ok) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G1"sv))
      .WillOnce(Return(mkG("G1","Alpha","T1")));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroup("T1","G1");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value()->Name(), "Alpha");
}

/* ================== UpdateGroup (rename) ================== */

TEST(GroupDelegateTest, UpdateGroup_ValidationErrors) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group g1; g1.Name() = "X";
  auto r1 = sut.UpdateGroup("T", g1);
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Group id required");

  domain::Group g2; g2.Id()="G";
  auto r2 = sut.UpdateGroup("T", g2);
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group name required");
}

TEST(GroupDelegateTest, UpdateGroup_NotFoundPaths) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group in; in.Id()="G1"; in.Name()="NewName";

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  auto r1 = sut.UpdateGroup("T0", in);
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Tournament doesn't exist");

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::ROUND_ROBIN)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G1"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));
  auto r2 = sut.UpdateGroup("T1", in);
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group doesn't exist");
}

TEST(GroupDelegateTest, UpdateGroup_Success_CallsUpdateWithNewName) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group in; in.Id()="G1"; in.Name()="NewName";

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G1"sv))
      .WillOnce(Return(mkG("G1","Old","T1")));
  EXPECT_CALL(*grepo, Update(::testing::_))
      .WillOnce(Invoke([](const domain::Group& g){
        EXPECT_EQ(g.Id(), "G1");
        EXPECT_EQ(g.Name(), "NewName");
        return g.Id();
      }));

  auto r = sut.UpdateGroup("T1", in);
  EXPECT_TRUE(r.has_value());
}

/* ================== RemoveGroup ================== */

TEST(GroupDelegateTest, RemoveGroup_Paths) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  auto r1 = sut.RemoveGroup("T0","G1");
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Tournament doesn't exist");

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G9"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));
  auto r2 = sut.RemoveGroup("T1","G9");
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group doesn't exist");
}

TEST(GroupDelegateTest, RemoveGroup_Success_CallsDelete) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G1"sv))
      .WillOnce(Return(mkG("G1","Alpha","T1")));
  EXPECT_CALL(*grepo, Delete("G1")).Times(1);

  auto r = sut.RemoveGroup("T1","G1");
  EXPECT_TRUE(r.has_value());
}

/* ================== UpdateTeams (batch) ================== */

TEST(GroupDelegateTest, UpdateTeams_CapacityAndDupChecks) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  auto r1 = sut.UpdateTeams("T0","G", {});
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Tournament doesn't exist");

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",1,1,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));
  auto r2 = sut.UpdateTeams("T1","G1", {});
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group doesn't exist");

  auto t2 = mkT("T2","Tour",1,1,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T2")).WillOnce(Return(t2));
  auto g2 = mkG("G2","Alpha","T2");
  g2->Teams().push_back(domain::Team{"E1","A"});
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T2"sv,"G2"sv)).WillOnce(Return(g2));
  auto r3 = sut.UpdateTeams("T2","G2", {domain::Team{"E2","B"}});
  ASSERT_FALSE(r3.has_value());
  EXPECT_EQ(r3.error(), "Group at max capacity");

  auto t3 = mkT("T3","Tour",1,5,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T3")).WillOnce(Return(t3));
  auto g3 = mkG("G3","Alpha","T3");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T3"sv,"G3"sv)).WillOnce(Return(g3));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T3"sv,"E1"sv))
      .WillOnce(Return(mkG("other","x","T3")));
  auto r4 = sut.UpdateTeams("T3","G3", {domain::Team{"E1","X"}});
  ASSERT_FALSE(r4.has_value());
  EXPECT_NE(r4.error().find("already exists"), std::string::npos);
}

TEST(GroupDelegateTest, UpdateTeams_Success_AddsEachTeam) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  EXPECT_CALL(*trepo, ReadById("T4"))
      .WillOnce(Return(mkT("T4","Tour",1,4,domain::TournamentType::ROUND_ROBIN)));
  auto g = mkG("G4","Alpha","T4");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T4"sv,"G4"sv)).WillOnce(Return(g));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T4"sv,"E1"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T4"sv,"E2"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*teamr, ReadById("E1"sv)).WillOnce(Return(mkTeam("E1","N1")));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","N2")));
  EXPECT_CALL(*grepo, UpdateGroupAddTeam("G4"sv, ::testing::_)).Times(2);

  auto r = sut.UpdateTeams("T4","G4",
                           {domain::Team{"E1","N1"}, domain::Team{"E2","N2"}});
  EXPECT_TRUE(r.has_value());
}

/* ================== AddTeamToGroup ================== */

TEST(GroupDelegateTest, AddTeamToGroup_PathsAndSuccess) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  // tournament missing
  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  auto r1 = sut.AddTeamToGroup("T0","G","E");
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Tournament doesn't exist");

  // group missing
  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",1,1,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv)).WillOnce(Return(nullptr));
  auto r2 = sut.AddTeamToGroup("T1","G1","E1");
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group doesn't exist");

  // team doesn't exist
  auto t2 = mkT("T2","Tour",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T2")).WillOnce(Return(t2));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T2"sv,"G2"sv)).WillOnce(Return(mkG("G2","Alpha","T2")));
  EXPECT_CALL(*teamr, ReadById("E9"sv)).WillOnce(Return(nullptr));
  auto r3 = sut.AddTeamToGroup("T2","G2","E9");
  ASSERT_FALSE(r3.has_value());
  EXPECT_EQ(r3.error(), "Team doesn't exist");

  // already exists
  auto t3 = mkT("T3","Tour",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T3")).WillOnce(Return(t3));
  auto g3 = mkG("G3","Alpha","T3");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T3"sv,"G3"sv)).WillOnce(Return(g3));
  EXPECT_CALL(*teamr, ReadById("E1"sv)).WillOnce(Return(mkTeam("E1","N")));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T3"sv,"E1"sv)).WillOnce(Return(mkG("X","x","T3")));
  auto r4 = sut.AddTeamToGroup("T3","G3","E1");
  ASSERT_FALSE(r4.has_value());
  EXPECT_NE(r4.error().find("already exists"), std::string::npos);

  // group full (⚠️ se llama ReadById antes de validar capacidad)
  auto t4 = mkT("T4","Tour",1,1,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T4")).WillOnce(Return(t4));
  auto g4 = mkG("G4","Alpha","T4");
  g4->Teams().push_back(domain::Team{"E0","A"}); // ya está lleno (capacidad 1)
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T4"sv,"G4"sv)).WillOnce(Return(g4));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","N2")));   // 👈 nuevo
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T4"sv,"E2"sv)).WillOnce(Return(nullptr)); // 👈 nuevo
  auto r5 = sut.AddTeamToGroup("T4","G4","E2");
  ASSERT_FALSE(r5.has_value());
  EXPECT_EQ(r5.error(), "Group is full");

  // success
  auto t5 = mkT("T5","Tour",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T5")).WillOnce(Return(t5));
  auto g5 = mkG("G5","Alpha","T5");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T5"sv,"G5"sv)).WillOnce(Return(g5));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","N2")));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T5"sv,"E2"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*grepo, UpdateGroupAddTeam("G5"sv, ::testing::_)).Times(1);
  auto r6 = sut.AddTeamToGroup("T5","G5","E2");
  EXPECT_TRUE(r6.has_value());
}
