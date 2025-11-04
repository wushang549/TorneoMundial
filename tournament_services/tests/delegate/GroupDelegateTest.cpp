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
#include "mocks/TeamRepositoryMock.h"

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

/* 
   Al método que procesa la creación de grupo para un torneo, 
   simular lectura de Tournament y validar que el valor que se le 
   transfiera a GroupRepository es el esperado. Validar que un 
   evento es generado. Validar que la respuesta sea ID del grupo
    */
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

/* 
   Al método que procesa la creación de grupo para un torneo,
   simular lectura de torneo por medio de TournamentRepository, 
   validar Group se le transfiera a GroupRepository es el esperado. 
   Simular error de inserción cuando ya existe el grupo usando expected
    */
TEST(GroupDelegateTest, CreateGroup_DuplicateGroup_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T3"))
      .WillOnce(Return(mkT("T3","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, Create(::testing::_))
      .WillOnce(Invoke([](const domain::Group& g){
        EXPECT_EQ(g.Name(), "DuplicateGroup");
        return std::string{}; // simulate failure in repository
      }));

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g; g.Name() = "DuplicateGroup";
  auto r = sut.CreateGroup("T3", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Failed to create group");
}

/* 
   Al método que procesa la creación de grupo para un torneo,
   simular lectura de torneo por medio de TournamentRepository, 
   validar Group se le transfiera a GroupRepository es el esperado. 
   Simular error de inserción cuando ya se llegó al número máximo de 
   equipos usando expected
    */
TEST(GroupDelegateTest, CreateGroup_MaxTeamsExceeded_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T4","Tour",2,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T4")).WillOnce(Return(t));
  EXPECT_CALL(*teamr, ReadById(::testing::_)).WillRepeatedly(Return(mkTeam("A","A"))); 

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g;
  g.Name() = "GFull";
  g.Teams() = {{"E1","T1"},{"E2","T2"},{"E3","T3"}}; // supera el límite (2)
  auto r = sut.CreateGroup("T4", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group at max capacity");
}

/* 
   Al método que procesa la creación de grupo para un torneo,
   simular lectura de Tournament inexistente y validar error
    */
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

/* 
   Al método que procesa la búsqueda de un grupo por ID y torneo por ID,
   validar que el valor que se le transfiera a GroupRepository es el esperado. 
   Simular el resultado con un objeto
    */
TEST(GroupDelegateTest, GetGroup_Ok) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv, "G1"sv))
      .WillOnce(Return(mkG("G1","Alpha","T1")));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroup("T1","G1");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value()->Name(), "Alpha");
}

/* 
   Al método que procesa la búsqueda de un grupo por ID y torneo por ID,
   validar que el valor que se le transfiera a GroupRepository es el esperado. 
   Simular el resultado nulo
    */
TEST(GroupDelegateTest, GetGroup_NotFound_ReturnsError) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1"))
      .WillOnce(Return(mkT("T1","Tour",2,4,domain::TournamentType::ROUND_ROBIN)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G404"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroup("T1","G404");
  ASSERT_FALSE(r.has_value());
}

/* 
   Al método que procesa la actualización de un grupo en un torneo,
   validar el valor de Group transferido a GroupRepository es el esperado. 
   Simular la actualización válida
    */
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

/* 
   Al método que procesa la actualización de un grupo en un torneo,
   validar el valor de Group transferido a GroupRepository es el esperado. 
   Simular ID no encontrado y regresar el valor usando expected
    */
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

/* 
   Al método que procesa la agregar un equipo a un grupo en un torneo, 
   validar Team se le transfiera a TeamRepository y GroupRepository es 
   el esperado, validar que un mensaje sea publicado. Simular la respuesta 
   de ID
    */
TEST(GroupDelegateTest, AddTeamToGroup_Success) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  auto t = mkT("T5","Tour",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T5")).WillOnce(Return(t));
  auto g = mkG("G5","Alpha","T5");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T5"sv,"G5"sv)).WillOnce(Return(g));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","N2")));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T5"sv,"E2"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*grepo, UpdateGroupAddTeam("G5"sv, ::testing::_)).Times(1);

  auto r = sut.AddTeamToGroup("T5","G5","E2");
  EXPECT_TRUE(r.has_value());
}

/* 
   Al método que procesa la agregar un equipo a un grupo en un torneo,
   validar Team sea transferido a TeamRepository. Simular que el equipo 
   no exista, regresar el valor usando expected
    */
TEST(GroupDelegateTest, AddTeamToGroup_TeamNotExist_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  auto t = mkT("T2","Tour",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T2")).WillOnce(Return(t));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T2"sv,"G2"sv)).WillOnce(Return(mkG("G2","Alpha","T2")));
  EXPECT_CALL(*teamr, ReadById("E9"sv)).WillOnce(Return(nullptr));

  auto r = sut.AddTeamToGroup("T2","G2","E9");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Team doesn't exist");
}

/* 
   Al método que procesa la agregar un equipo a un grupo en un torneo,
   validar Team sea transferido a TeamRepository y GroupRepository. 
   Simular que el grupo esté lleno, regresar el valor usando expected
    */
TEST(GroupDelegateTest, AddTeamToGroup_GroupFull_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  auto t = mkT("T4","Tour",1,1,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T4")).WillOnce(Return(t));
  auto g = mkG("G4","Alpha","T4");
  g->Teams().push_back(domain::Team{"E0","A"}); // grupo lleno
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T4"sv,"G4"sv)).WillOnce(Return(g));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","N2")));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T4"sv,"E2"sv)).WillOnce(Return(nullptr));

  auto r = sut.AddTeamToGroup("T4","G4","E2");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group is full");
}
// ---- CreateGroup: team missing, team already in tournament ----
TEST(GroupDelegateTest, CreateGroup_TeamMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T1","Tour",2,4,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  // first team does not exist
  EXPECT_CALL(*teamr, ReadById("E404"sv)).WillOnce(Return(nullptr));

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g; g.Name() = "G"; g.Teams() = { {"E404","Ghost"} };
  auto r = sut.CreateGroup("T1", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Team doesn't exist");
}

TEST(GroupDelegateTest, CreateGroup_TeamAlreadyInTournament_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T1","Tour",2,4,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  EXPECT_CALL(*teamr, ReadById("E1"sv)).WillOnce(Return(mkTeam("E1","X")));
  // already present in tournament
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E1"sv))
      .WillOnce(Return(mkG("GZ","Z","T1")));

  GroupDelegate sut{trepo, grepo, teamr};
  domain::Group g; g.Name()="G"; g.Teams()={{"E1","X"}};
  auto r = sut.CreateGroup("T1", g);
  ASSERT_FALSE(r.has_value());
  EXPECT_THAT(r.error(), ::testing::HasSubstr("Team E1 already exists in tournament T1"));
}

// ---- GetGroups: tournament missing and success pass-through ----
TEST(GroupDelegateTest, GetGroups_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("TX")).WillOnce(Return(nullptr));
  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroups("TX");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

TEST(GroupDelegateTest, GetGroups_Success_ReturnsVector) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T2")).WillOnce(Return(mkT("T2","Tour",1,3,domain::TournamentType::NFL)));
  auto g1 = mkG("G1","A","T2");
  auto g2 = mkG("G2","B","T2");
  EXPECT_CALL(*grepo, FindByTournamentId("T2"sv))
      .WillOnce(Return(std::vector{g1, g2}));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroups("T2");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->size(), 2u);
}

// ---- GetGroup: tournament missing ----
TEST(GroupDelegateTest, GetGroup_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("TZ")).WillOnce(Return(nullptr));
  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.GetGroup("TZ","G1");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

// ---- UpdateGroup: input validation and repo update failure ----
TEST(GroupDelegateTest, UpdateGroup_IdRequiredAndNameRequired) {
  auto trepo = std::make_shared<NiceMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group noId; noId.Name()="N";
  auto r1 = sut.UpdateGroup("T1", noId);
  ASSERT_FALSE(r1.has_value());
  EXPECT_EQ(r1.error(), "Group id required");

  domain::Group noName; noName.Id()="G1";
  auto r2 = sut.UpdateGroup("T1", noName);
  ASSERT_FALSE(r2.has_value());
  EXPECT_EQ(r2.error(), "Group name required");
}

TEST(GroupDelegateTest, UpdateGroup_UpdateReturnsEmpty_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();
  GroupDelegate sut{trepo, grepo, teamr};

  domain::Group in; in.Id()="G1"; in.Name()="N";
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(mkT("T1","T",1,2,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv))
      .WillOnce(Return(mkG("G1","Old","T1")));
  EXPECT_CALL(*grepo, Update(::testing::_))
      .WillOnce(Return(std::string{})); // simulate failure

  auto r = sut.UpdateGroup("T1", in);
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Failed to update group");
}

// ---- RemoveGroup: tournament/group missing and success ----
TEST(GroupDelegateTest, RemoveGroup_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.RemoveGroup("T0","G1");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

TEST(GroupDelegateTest, RemoveGroup_GroupMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(mkT("T1","T",1,2,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G404"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.RemoveGroup("T1","G404");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group doesn't exist");
}

// ---- UpdateTeams: all error branches and success ----
TEST(GroupDelegateTest, UpdateTeams_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.UpdateTeams("T0","G1", std::vector<domain::Team>{{"E1","A"}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

TEST(GroupDelegateTest, UpdateTeams_GroupMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(mkT("T1","T",1,2,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G404"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.UpdateTeams("T1","G404", std::vector<domain::Team>{{"E1","A"}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group doesn't exist");
}

TEST(GroupDelegateTest, UpdateTeams_ExceedsCapacity_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  auto t = mkT("T1","T",1,2,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  auto g = mkG("G1","A","T1"); g->Teams().push_back(domain::Team{"E0","X"}); // current = 1
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv)).WillOnce(Return(g));

  GroupDelegate sut{trepo, grepo, teamr};
  // incoming = 2, total = 3 > max(2)
  auto r = sut.UpdateTeams("T1","G1", std::vector<domain::Team>{{"E1","A"},{"E2","B"}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group at max capacity");
}

TEST(GroupDelegateTest, UpdateTeams_DuplicateInTournament_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  auto t = mkT("T1","T",1,3,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  auto g = mkG("G1","A","T1"); // current = 0
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv)).WillOnce(Return(g));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E1"sv))
      .WillOnce(Return(mkG("Gx","x","T1"))); // duplicate found

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.UpdateTeams("T1","G1", std::vector<domain::Team>{{"E1","A"}});
  ASSERT_FALSE(r.has_value());
  EXPECT_THAT(r.error(), ::testing::HasSubstr("Team E1 already exists in tournament T1"));
}

TEST(GroupDelegateTest, UpdateTeams_TeamMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T1","T",1,3,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  auto g = mkG("G1","A","T1");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv)).WillOnce(Return(g));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E404"sv))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(*teamr, ReadById("E404"sv))
      .WillOnce(Return(nullptr)); // missing

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.UpdateTeams("T1","G1", std::vector<domain::Team>{{"E404","X"}});
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Team E404 doesn't exist");
}

TEST(GroupDelegateTest, UpdateTeams_Success_AddsAllTeams) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T1","T",1,4,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  auto g = mkG("G1","A","T1");
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv))
      .WillOnce(Return(g));
  // no duplicates
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E1"sv))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E2"sv))
      .WillOnce(Return(nullptr));
  // both teams exist
  EXPECT_CALL(*teamr, ReadById("E1"sv)).WillOnce(Return(mkTeam("E1","A")));
  EXPECT_CALL(*teamr, ReadById("E2"sv)).WillOnce(Return(mkTeam("E2","B")));
  // both added
  EXPECT_CALL(*grepo, UpdateGroupAddTeam("G1"sv, ::testing::_)).Times(2);

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.UpdateTeams("T1","G1", std::vector<domain::Team>{{"E1","A"},{"E2","B"}});
  EXPECT_TRUE(r.has_value());
}

// ---- AddTeamToGroup: tournament/group missing and already exists ----
TEST(GroupDelegateTest, AddTeamToGroup_TournamentMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T0")).WillOnce(Return(nullptr));
  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.AddTeamToGroup("T0","G1","E1");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Tournament doesn't exist");
}

TEST(GroupDelegateTest, AddTeamToGroup_GroupMissing_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<NiceMock<TeamRepositoryMock>>();

  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(mkT("T1","T",1,2,domain::TournamentType::NFL)));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G404"sv))
      .WillOnce(Return(std::shared_ptr<domain::Group>{}));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.AddTeamToGroup("T1","G404","E1");
  ASSERT_FALSE(r.has_value());
  EXPECT_EQ(r.error(), "Group doesn't exist");
}

TEST(GroupDelegateTest, AddTeamToGroup_AlreadyExists_ReturnsError) {
  auto trepo = std::make_shared<StrictMock<TournamentRepositoryMock>>();
  auto grepo = std::make_shared<StrictMock<GroupRepositoryMock>>();
  auto teamr = std::make_shared<StrictMock<TeamRepositoryMock>>();

  auto t = mkT("T1","T",1,3,domain::TournamentType::NFL);
  EXPECT_CALL(*trepo, ReadById("T1")).WillOnce(Return(t));
  EXPECT_CALL(*grepo, FindByTournamentIdAndGroupId("T1"sv,"G1"sv))
      .WillOnce(Return(mkG("G1","A","T1")));
  EXPECT_CALL(*teamr, ReadById("E1"sv)).WillOnce(Return(mkTeam("E1","A")));
  // duplicate in tournament
  EXPECT_CALL(*grepo, FindByTournamentIdAndTeamId("T1"sv,"E1"sv))
      .WillOnce(Return(mkG("GZ","Z","T1")));

  GroupDelegate sut{trepo, grepo, teamr};
  auto r = sut.AddTeamToGroup("T1","G1","E1");
  ASSERT_FALSE(r.has_value());
  EXPECT_THAT(r.error(), ::testing::HasSubstr("Team E1 already exists in tournament T1"));
}