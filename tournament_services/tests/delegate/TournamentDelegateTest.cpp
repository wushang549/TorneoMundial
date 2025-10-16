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

static std::shared_ptr<domain::Tournament> mkT(
  const std::string& id, const std::string& name,
  int groups, int maxPerGroup, domain::TournamentType type)
{
  domain::TournamentFormat fmt{groups, maxPerGroup, type};
  auto p = std::make_shared<domain::Tournament>(name, fmt);
  p->Id() = id;
  return p;
}

TEST(TournamentDelegateTest, Create_ReturnsRepoId){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_)).WillOnce(Return(std::string{"t-001"}));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{2,4,domain::TournamentType::ROUND_ROBIN};
  auto obj = std::make_shared<domain::Tournament>("Alpha", fmt);
  EXPECT_EQ(sut.CreateTournament(obj), "t-001");
}

TEST(TournamentDelegateTest, ReadAll_Empty){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
  TournamentDelegate sut{repo};
  EXPECT_TRUE(sut.ReadAll().empty());
}

TEST(TournamentDelegateTest, ReadById_Found){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, ReadById("x9")).WillOnce(Return(mkT("x9","X",1,8,domain::TournamentType::NFL)));
  TournamentDelegate sut{repo};
  auto t = sut.ReadById("x9");
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->Name(), "X");
}

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

TEST(TournamentDelegateTest, Update_RepoThrows_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Update(::testing::_))
    .WillOnce(Invoke([](const domain::Tournament&){ throw std::runtime_error("db"); return std::string{}; }));
  TournamentDelegate sut{repo};
  domain::TournamentFormat fmt{1,4,domain::TournamentType::NFL};
  domain::Tournament in{"Omega", fmt};
  EXPECT_FALSE(sut.UpdateTournament("id", in));
}

TEST(TournamentDelegateTest, Delete_Success_ReturnsTrue){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D1")).Times(1);
  TournamentDelegate sut{repo};
  EXPECT_TRUE(sut.DeleteTournament("D1"));
}

TEST(TournamentDelegateTest, Delete_RepoThrows_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTournamentRepository>>();
  EXPECT_CALL(*repo, Delete("D2"))
    .WillOnce(Invoke([](std::string){ throw std::runtime_error("fk"); }));
  TournamentDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTournament("D2"));
}
