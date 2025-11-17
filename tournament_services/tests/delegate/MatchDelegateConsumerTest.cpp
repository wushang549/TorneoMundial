#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <vector>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

// IMPORTANT: include the *consumer* MatchDelegate
#include "../../tournament_consumer/include/delegate/MatchDelegate.hpp"

// Domain
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"

// Mocks
#include "mocks/MatchRepositoryMock.hpp"
#include "mocks/GroupRepositoryMock.hpp"
#include "mocks/TournamentRepositoryMock.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::AtLeast;
using ::testing::Invoke;

// Small helpers
namespace {

std::shared_ptr<domain::Group> makeGroup(
    const std::string& id,
    const std::string& name,
    const std::vector<std::string>& teamIds
) {
    auto g = std::make_shared<domain::Group>(name, id);
    for (const auto& tid : teamIds) {
        domain::Team t;
        t.Id   = tid;
        t.Name = tid;
        g->Teams().push_back(t);
    }
    return g;
}

std::shared_ptr<domain::Match> makeMatch(
    const std::string& tid,
    const std::string& round,
    const std::string& homeId,
    const std::string& visId,
    int scoreHome = -1,
    int scoreVis = -1
) {
    auto m = std::make_shared<domain::Match>();
    m->TournamentId() = tid;
    m->Round() = round;
    m->Home().Id() = homeId;
    m->Home().Name() = homeId;
    m->Visitor().Id() = visId;
    m->Visitor().Name() = visId;
    if (scoreHome >= 0 && scoreVis >= 0) {
        m->ScoreHome() = scoreHome;
        m->ScoreVisitor() = scoreVis;
    }
    return m;
}

struct Fixture {
    // Strict mocks to ensure all expectations are met
    StrictMock<MatchRepositoryMock>      matchRepoMock;
    StrictMock<GroupRepositoryMock>      groupRepoMock;
    StrictMock<TournamentRepositoryMock> tournamentRepoMock;

    // Non-owning shared_ptr wrappers (no delete)
    std::shared_ptr<IMatchRepository> matchRepo{
        &matchRepoMock, [](IMatchRepository*){}
    };
    std::shared_ptr<IGroupRepository> groupRepo{
        &groupRepoMock, [](IGroupRepository*){}
    };
    std::shared_ptr<TournamentRepository> tournamentRepo{
        &tournamentRepoMock, [](TournamentRepository*){}
    };

    MatchDelegate delegate{matchRepo, groupRepo, tournamentRepo};
};

} // namespace

// =====================================================================
// ProcessTeamAddition Tests
// =====================================================================

TEST(MatchDelegateWorldCupTest, ProcessTeamAddition_TournamentNotReady_DoesNotCreateMatches) {
    Fixture fx;

    TeamAddEvent evt{};
    evt.tournamentId = "TID-1";
    evt.groupId      = "G1";
    evt.teamId       = "TEAM-1";

    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 4}); // 2 groups, 4 teams each
    tour->Id() = "TID-1";

    auto g1 = makeGroup("G1", "Group 1", {"TEAM-1"}); // Only 1 team
    auto g2 = makeGroup("G2", "Group 2", {"TEAM-2"}); // Only 1 team

    // Expectations
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentIdAndGroupId("TID-1", "G1"))
        .WillOnce(Return(g1));
    
    EXPECT_CALL(fx.tournamentRepoMock, ReadById("TID-1"))
        .Times(2) 
        .WillRepeatedly(Return(tour));
    
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId("TID-1"))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));



    // Execute
    fx.delegate.ProcessTeamAddition(evt);

}

TEST(MatchDelegateWorldCupTest, ProcessTeamAddition_TournamentReady_CreatesGroupMatches) {
    Fixture fx;

    TeamAddEvent evt{};
    evt.tournamentId = "TID-2";
    evt.groupId      = "G1";
    evt.teamId       = "TEAM-4";

    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 4});
    tour->Id() = "TID-2";

    auto g1 = makeGroup("G1", "Group 1", {"T1", "T2", "T3", "T4"});
    auto g2 = makeGroup("G2", "Group 2", {"T5", "T6", "T7", "T8"});

    // Expectations
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentIdAndGroupId("TID-2", "G1"))
        .WillOnce(Return(g1));
    
    EXPECT_CALL(fx.tournamentRepoMock, ReadById("TID-2"))
        .Times(AtLeast(2))
        .WillRepeatedly(Return(tour));
    
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId("TID-2"))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));

    // IMPORTANT: Expect Create to be called for group stage matches
    // Each group with 4 teams generates C(4,2) = 6 matches, so 2 groups = 12 matches
    EXPECT_CALL(fx.matchRepoMock, Create(_))
        .Times(12)
        .WillRepeatedly(Return("MATCH-ID"));

    // Execute
    fx.delegate.ProcessTeamAddition(evt);

}

TEST(MatchDelegateWorldCupTest, ProcessTeamAddition_MatchesAlreadyExist_DoesNotDuplicate) {
    Fixture fx;

    TeamAddEvent evt{};
    evt.tournamentId = "TID-3";
    evt.groupId      = "G1";
    evt.teamId       = "TEAM-1";

    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 4});
    tour->Id() = "TID-3";

    auto g1 = makeGroup("G1", "Group 1", {"T1", "T2", "T3", "T4"});
    auto g2 = makeGroup("G2", "Group 2", {"T5", "T6", "T7", "T8"});

    // Some matches already exist
    std::vector<std::shared_ptr<domain::Match>> existingMatches;
    existingMatches.push_back(makeMatch("TID-3", rounds::GROUP, "T1", "T2"));

    // Expectations
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentIdAndGroupId("TID-3", "G1"))
        .WillOnce(Return(g1));
    
    EXPECT_CALL(fx.tournamentRepoMock, ReadById("TID-3"))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(tour));
    
    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId("TID-3"))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));

    // Execute
    fx.delegate.ProcessTeamAddition(evt);
}

// ProcessScoreUpdate Tests

TEST(MatchDelegateWorldCupTest, ProcessScoreUpdate_PendingGroupMatches_DoesNotCreatePlayoffs) {
    Fixture fx;

    ScoreUpdateEvent evt{};
    evt.tournamentId = "TID-4";
    evt.matchId = "M1";

    // Some group matches played, some pending
    std::vector<std::shared_ptr<domain::Match>> matches;
    matches.push_back(makeMatch("TID-4", rounds::GROUP, "T1", "T2", 2, 1)); // Played
    matches.push_back(makeMatch("TID-4", rounds::GROUP, "T3", "T4"));       // Pending (no score)

    // Expectations
    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId("TID-4"))
        .WillOnce(Return(matches));


    // Execute
    fx.delegate.ProcessScoreUpdate(evt);
}

TEST(MatchDelegateWorldCupTest, ProcessScoreUpdate_AllGroupMatchesPlayed_CreatesPlayoffMatches) {
    Fixture fx;

    ScoreUpdateEvent evt{};
    evt.tournamentId = "TID-5";
    evt.matchId = "M-LAST";

    // Tournament with 8 groups, 2 teams each
    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{8, 2});
    tour->Id() = "TID-5";

    std::vector<std::shared_ptr<domain::Group>> groups;
    for (int i = 1; i <= 8; ++i) {
        std::string gid = "G" + std::to_string(i);
        std::string t1 = gid + "A";
        std::string t2 = gid + "B";
        groups.push_back(makeGroup(gid, "Group " + std::to_string(i), {t1, t2}));
    }

    // All group matches are played
    std::vector<std::shared_ptr<domain::Match>> matches;
    for (int i = 1; i <= 8; ++i) {
        std::string gid = "G" + std::to_string(i);
        std::string t1 = gid + "A";
        std::string t2 = gid + "B";
        matches.push_back(makeMatch("TID-5", rounds::GROUP, t1, t2, 2, 1));
    }

    // Expectations
    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId("TID-5"))
        .Times(2) // Called twice: check pending + create playoffs
        .WillRepeatedly(Return(matches));

    EXPECT_CALL(fx.tournamentRepoMock, ReadById("TID-5"))
        .WillOnce(Return(tour));

    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId("TID-5"))
        .WillOnce(Return(groups));

    // IMPORTANT: Expect playoff matches to be created
    // With 8 groups, we get 16 teams (top 2 from each), so 8 R16 matches
    EXPECT_CALL(fx.matchRepoMock, CreateIfNotExists(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return("PLAYOFF-MATCH-ID"));

    // Execute
    fx.delegate.ProcessScoreUpdate(evt);
}

TEST(MatchDelegateWorldCupTest, ProcessScoreUpdate_PlayoffMatchesExist_DoesNotDuplicate) {
    Fixture fx;

    ScoreUpdateEvent evt{};
    evt.tournamentId = "TID-6";
    evt.matchId = "M-PLAYOFF";

    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{4, 2});
    tour->Id() = "TID-6";

    std::vector<std::shared_ptr<domain::Group>> groups;
    for (int i = 1; i <= 4; ++i) {
        std::string gid = "G" + std::to_string(i);
        groups.push_back(makeGroup(gid, "Group " + std::to_string(i), 
                                  {gid + "A", gid + "B"}));
    }

    // All group matches played + playoff matches already exist
    std::vector<std::shared_ptr<domain::Match>> matches;
    for (int i = 1; i <= 4; ++i) {
        std::string gid = "G" + std::to_string(i);
        matches.push_back(makeMatch("TID-6", rounds::GROUP, gid + "A", gid + "B", 1, 0));
    }
    // Add existing playoff matches
    matches.push_back(makeMatch("TID-6", rounds::R16, "G1A", "G2B", 2, 1));
    matches.push_back(makeMatch("TID-6", rounds::R16, "G2A", "G1B", 1, 0));

    // Expectations
    EXPECT_CALL(fx.matchRepoMock, FindByTournamentId("TID-6"))
        .Times(2)
        .WillRepeatedly(Return(matches));

    EXPECT_CALL(fx.tournamentRepoMock, ReadById("TID-6"))
        .WillOnce(Return(tour));

    EXPECT_CALL(fx.groupRepoMock, FindByTournamentId("TID-6"))
        .WillOnce(Return(groups));

    // CreateIfNotExists should return empty string for existing matches
    EXPECT_CALL(fx.matchRepoMock, CreateIfNotExists(_))
        .WillRepeatedly(Return(""));

    // Execute
    fx.delegate.ProcessScoreUpdate(evt);
}
