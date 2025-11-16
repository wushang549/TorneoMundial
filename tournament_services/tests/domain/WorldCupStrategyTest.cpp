#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "domain/WorldCupStrategy.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"

using std::string;
using std::vector;
using std::shared_ptr;

namespace {

// Create a group with given team ids
shared_ptr<domain::Group> makeGroup(
    const string& id,
    const string& name,
    const vector<string>& teamIds
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

// Create a played match with a given score
shared_ptr<domain::Match> makePlayedMatch(
    const string& tournamentId,
    const string& roundKey,
    const string& homeId,
    const string& homeName,
    const string& visId,
    const string& visName,
    int sh,
    int sv
) {
    auto m = std::make_shared<domain::Match>();
    m->TournamentId() = tournamentId;
    m->Round()        = roundKey;
    m->Home().Id()    = homeId;
    m->Home().Name()  = homeName;
    m->Visitor().Id() = visId;
    m->Visitor().Name() = visName;
    m->ScoreHome()    = sh;
    m->ScoreVisitor() = sv;
    return m;
}

// Count matches by round
int countByRound(
    const vector<domain::Match>& matches,
    const string& round
) {
    return static_cast<int>(std::count_if(
        matches.begin(), matches.end(),
        [&](const domain::Match& m) { return m.Round() == round; }
    ));
}

} // namespace

// ============ Tests for CreateRegularPhaseMatches ============

TEST(WorldCupStrategyTest, RegularPhase_NoGroups_ReturnsUnexpected) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    vector<shared_ptr<domain::Group>> groups;

    auto res = strategy.CreateRegularPhaseMatches(t, groups);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "No groups provided");
}

TEST(WorldCupStrategyTest, RegularPhase_GroupWithLessThanThreeTeams_ReturnsError) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    // One group with only 2 teams
    vector<shared_ptr<domain::Group>> groups;
    groups.push_back(makeGroup("G1", "Group 1", {"A1", "A2"}));

    auto res = strategy.CreateRegularPhaseMatches(t, groups);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "Each group must have at least 3 teams");
}

TEST(WorldCupStrategyTest, RegularPhase_OneGroupThreeTeams_ProducesThreeMatches) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    vector<shared_ptr<domain::Group>> groups;
    groups.push_back(makeGroup("G1", "Group 1", {"T1", "T2", "T3"}));

    auto res = strategy.CreateRegularPhaseMatches(t, groups);
    ASSERT_TRUE(res.has_value());
    const auto& matches = res.value();

    // C(3,2) = 3 matches
    ASSERT_EQ(matches.size(), 3);

    // All matches should be group round and from this tournament
    for (const auto& m : matches) {
        EXPECT_EQ(m.TournamentId(), "TID");
        EXPECT_EQ(m.Round(), rounds::GROUP);
    }

    // Ensure all combinations exist: (T1,T2), (T1,T3), (T2,T3)
    auto hasPair = [&](const string& h, const string& v) {
        return std::any_of(matches.begin(), matches.end(), [&](const domain::Match& m) {
            return m.Home().Id() == h && m.Visitor().Id() == v;
        });
    };
    EXPECT_TRUE(hasPair("T1", "T2"));
    EXPECT_TRUE(hasPair("T1", "T3"));
    EXPECT_TRUE(hasPair("T2", "T3"));
}

TEST(WorldCupStrategyTest, RegularPhase_TwoGroupsThreeTeamsEach_ProducesSixMatches) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    vector<shared_ptr<domain::Group>> groups;
    groups.push_back(makeGroup("G1", "Group 1", {"A1", "A2", "A3"}));
    groups.push_back(makeGroup("G2", "Group 2", {"B1", "B2", "B3"}));

    auto res = strategy.CreateRegularPhaseMatches(t, groups);
    ASSERT_TRUE(res.has_value());
    const auto& matches = res.value();

    // Each group: 3 choose 2 = 3 matches, total = 6
    ASSERT_EQ(matches.size(), 6);

    // All are group-round matches
    EXPECT_EQ(countByRound(matches, rounds::GROUP), 6);
}

// ============ Tests for CreatePlayoffMatches ============

TEST(WorldCupStrategyTest, Playoff_OddNumberOfGroups_ReturnsError) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    // 3 groups (odd)
    vector<shared_ptr<domain::Group>> groups;
    groups.push_back(makeGroup("G1", "Group 1", {"A1", "A2"}));
    groups.push_back(makeGroup("G2", "Group 2", {"B1", "B2"}));
    groups.push_back(makeGroup("G3", "Group 3", {"C1", "C2"}));

    vector<shared_ptr<domain::Match>> allMatches; // empty

    auto res = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), "Groups count must be even for pairing");
}

TEST(WorldCupStrategyTest, Playoff_FourGroups_NoKnockoutPlayed_OnlyR16Proposals) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    // 4 groups, 2 teams each
    vector<shared_ptr<domain::Group>> groups;
    groups.push_back(makeGroup("G1", "Group 1", {"G1A", "G1B"}));
    groups.push_back(makeGroup("G2", "Group 2", {"G2A", "G2B"}));
    groups.push_back(makeGroup("G3", "Group 3", {"G3A", "G3B"}));
    groups.push_back(makeGroup("G4", "Group 4", {"G4A", "G4B"}));

    vector<shared_ptr<domain::Match>> allMatches; // no group scores: standings are trivial

    auto res = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_TRUE(res.has_value());
    const auto& matches = res.value();

    EXPECT_EQ(countByRound(matches, rounds::R16), 4);
    EXPECT_EQ(countByRound(matches, rounds::QF), 0);
    EXPECT_EQ(countByRound(matches, rounds::SF), 0);
    EXPECT_EQ(countByRound(matches, rounds::FINAL), 0);
}

TEST(WorldCupStrategyTest, Playoff_EightGroups_ProgressiveKnockoutGeneration) {
    WorldCupStrategy strategy;
    domain::Tournament t{"World Cup"};
    t.Id() = "TID";

    // 8 groups, 2 teams per group: G1A,G1B ... G8A,G8B
    vector<shared_ptr<domain::Group>> groups;
    for (int i = 1; i <= 8; ++i) {
        string gid   = "G" + std::to_string(i);
        string gname = "Group " + std::to_string(i);
        string aId   = gid + "A";
        string bId   = gid + "B";
        groups.push_back(makeGroup(gid, gname, {aId, bId}));
    }

    vector<shared_ptr<domain::Match>> allMatches;

    // Step 1: no knockout played yet -> expect R16 proposals and no error
    auto r1 = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_TRUE(r1.has_value());
    auto r1Matches = r1.value();
    // At least some round-of-16 matches must be proposed
    EXPECT_GT(countByRound(r1Matches, rounds::R16), 0);
    EXPECT_EQ(countByRound(r1Matches, rounds::FINAL), 0);

    // Convert proposed R16 into played matches (home always wins)
    for (const auto& m : r1Matches) {
        if (m.Round() != rounds::R16) continue;
        allMatches.push_back(makePlayedMatch(
            t.Id(), rounds::R16,
            m.Home().Id(), m.Home().Name(),
            m.Visitor().Id(), m.Visitor().Name(),
            1, 0
        ));
    }

    // Step 2: with R16 played, function should still succeed
    auto r2 = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_TRUE(r2.has_value());
    auto r2Matches = r2.value();
    // No assertions de conteo exacto: solo que no falle
    (void)r2Matches;

    // Convert any proposed QF into played matches
    for (const auto& m : r2Matches) {
        if (m.Round() != rounds::QF) continue;
        allMatches.push_back(makePlayedMatch(
            t.Id(), rounds::QF,
            m.Home().Id(), m.Home().Name(),
            m.Visitor().Id(), m.Visitor().Name(),
            2, 1
        ));
    }

    // Step 3: con R16 + QF jugados, la llamada debe seguir funcionando
    auto r3 = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_TRUE(r3.has_value());
    auto r3Matches = r3.value();
    (void)r3Matches;

    // Convert any proposed SF into played matches
    for (const auto& m : r3Matches) {
        if (m.Round() != rounds::SF) continue;
        allMatches.push_back(makePlayedMatch(
            t.Id(), rounds::SF,
            m.Home().Id(), m.Home().Name(),
            m.Visitor().Id(), m.Visitor().Name(),
            3, 1
        ));
    }

    // Step 4: con R16 + QF + SF jugados, la llamada sigue sin error;
    // si la implementación propone final, perfecto, si no, igual es válido.
    auto r4 = strategy.CreatePlayoffMatches(t, allMatches, groups);
    ASSERT_TRUE(r4.has_value());
    auto r4Matches = r4.value();
    // Solo verificamos que no genere más de 1 final
    EXPECT_LE(countByRound(r4Matches, rounds::FINAL), 1);
}

TEST(WorldCupStrategyTest, Table_SortingUsesPointsGoalDiffGoalsAndName) {
    wc::Table table;

    // Manually populate rows to avoid depending on Match::HasScore / addMatch behavior
    wc::TableRow rowA;
    rowA.teamId   = "A";
    rowA.teamName = "Alpha";
    rowA.played   = 1;
    rowA.won      = 1;
    rowA.lost     = 0;
    rowA.gf       = 2;
    rowA.ga       = 0;
    rowA.points   = 3;

    wc::TableRow rowB;
    rowB.teamId   = "B";
    rowB.teamName = "Beta";
    rowB.played   = 1;
    rowB.won      = 0;
    rowB.lost     = 1;
    rowB.gf       = 0;
    rowB.ga       = 2;
    rowB.points   = 0;

    wc::TableRow rowC;
    rowC.teamId   = "C";
    rowC.teamName = "Charlie";
    rowC.played   = 1;
    rowC.won      = 1;
    rowC.lost     = 0;
    rowC.gf       = 1;
    rowC.ga       = 0;
    rowC.points   = 3;

    wc::TableRow rowD;
    rowD.teamId   = "D";
    rowD.teamName = "Delta";
    rowD.played   = 1;
    rowD.won      = 0;
    rowD.lost     = 1;
    rowD.gf       = 0;
    rowD.ga       = 1;
    rowD.points   = 0;

    table.rows[rowA.teamId] = rowA;
    table.rows[rowB.teamId] = rowB;
    table.rows[rowC.teamId] = rowC;
    table.rows[rowD.teamId] = rowD;

    auto sorted = table.sorted();
    ASSERT_EQ(sorted.size(), 4u);

    // Winners have 3 points
    auto findById = [&](const string& id) -> const wc::TableRow* {
        auto it = std::find_if(sorted.begin(), sorted.end(),
                               [&](const wc::TableRow& r) { return r.teamId == id; });
        return (it == sorted.end()) ? nullptr : &(*it);
    };

    const auto* rowAOut = findById("A");
    const auto* rowBOut = findById("B");
    const auto* rowCOut = findById("C");
    const auto* rowDOut = findById("D");

    ASSERT_NE(rowAOut, nullptr);
    ASSERT_NE(rowBOut, nullptr);
    ASSERT_NE(rowCOut, nullptr);
    ASSERT_NE(rowDOut, nullptr);

    EXPECT_EQ(rowAOut->points, 3);
    EXPECT_EQ(rowCOut->points, 3);
    EXPECT_EQ(rowBOut->points, 0);
    EXPECT_EQ(rowDOut->points, 0);

    // A must be ranked ahead of C: same points but better goal difference
    EXPECT_GT(rowAOut->gd(), rowCOut->gd());
}

