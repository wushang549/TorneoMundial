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

static std::shared_ptr<domain::Team> mkT(std::string id, std::string name){
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

TEST(TeamDelegateTest, GetAll_Empty){
  auto repo = std::make_shared<NiceMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  TeamDelegate sut{repo};
  EXPECT_TRUE(sut.GetAllTeams().empty());
}

TEST(TeamDelegateTest, GetTeam_UsesReadByIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(mkT("X","XName")));
  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X");
  ASSERT_NE(t,nullptr);
  EXPECT_EQ(t->Name,"XName");
}

TEST(TeamDelegateTest, SaveTeam_ReturnsIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_)).WillOnce(Return("id-1"sv));
  TeamDelegate sut{repo};
  domain::Team in{"","Nuevo"};
  EXPECT_EQ(sut.SaveTeam(in), "id-1");
}

TEST(TeamDelegateTest, Update_NotFound_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById(An<std::string_view>())).WillOnce(Return(nullptr));
  EXPECT_CALL(*repo, Update(::testing::_)).Times(0);
  TeamDelegate sut{repo};
  domain::Team in{"U1","Name"};
  EXPECT_FALSE(sut.UpdateTeam("U1", in));
}

TEST(TeamDelegateTest, Update_Found_CallsUpdateAndTrue){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("U2"sv)).WillOnce(Return(mkT("U2","Old")));
  EXPECT_CALL(*repo, Update(::testing::_)).WillOnce(Return("U2"sv));
  TeamDelegate sut{repo};
  domain::Team in{"U2","New"};
  EXPECT_TRUE(sut.UpdateTeam("U2", in));
}

TEST(TeamDelegateTest, Delete_NotFound_ReturnsFalse){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("D1"sv)).WillOnce(Return(nullptr));
  EXPECT_CALL(*repo, Delete(An<std::string_view>())).Times(0);
  TeamDelegate sut{repo};
  EXPECT_FALSE(sut.DeleteTeam("D1"));
}

TEST(TeamDelegateTest, Delete_Success_ReturnsTrue){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  TeamDelegate sut{repo};
  InSequence seq;
  EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(mkT("D2","ToDelete")));
  EXPECT_CALL(*repo, Delete("D2"sv)).Times(1);
  EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(nullptr));
  EXPECT_TRUE(sut.DeleteTeam("D2"));
}
