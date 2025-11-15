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

using ::testing::NiceMock;

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

struct Fixture {
    // Mocks live on stack
    NiceMock<MatchRepositoryMock>      matchRepoMock;
    NiceMock<GroupRepositoryMock>      groupRepoMock;
    NiceMock<TournamentRepositoryMock> tournamentRepoMock;

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

// ---------------------------------------------------------------------
// ProcessTeamAddition: tournament not ready (minimal smoke test)
// ---------------------------------------------------------------------
TEST(MatchDelegateWorldCupTest,
     ProcessTeamAddition_TournamentNotReady_DoesNotCrash) {
    Fixture fx;

    TeamAddEvent evt{};
    evt.tournamentId = "TID-1";
    evt.groupId      = "G1";
    evt.teamId       = "TEAM-1";

    // We only verify that the call does not throw or crash.
    fx.delegate.ProcessTeamAddition(evt);
}

// ---------------------------------------------------------------------
// ProcessTeamAddition: tournament "ready" scenario (smoke test)
// ---------------------------------------------------------------------
TEST(MatchDelegateWorldCupTest,
     ProcessTeamAddition_TournamentReady_DoesNotCrash) {
    Fixture fx;

    TeamAddEvent evt{};
    evt.tournamentId = "TID-2";
    evt.groupId      = "G1";
    evt.teamId       = "ANY-TEAM";

    // Example setup (not strictly required by expectations)
    auto tour = std::make_shared<domain::Tournament>(
        "World Cup", domain::TournamentFormat{2, 3});
    tour->Id() = "TID-2";

    auto g1 = makeGroup("G1", "Group 1", {"A1", "A2", "A3"});
    auto g2 = makeGroup("G2", "Group 2", {"B1", "B2", "B3"});
    (void)tour;
    (void)g1;
    (void)g2;

    fx.delegate.ProcessTeamAddition(evt);
}

// ---------------------------------------------------------------------
// ProcessScoreUpdate: there are pending group matches scenario (smoke test)
// ---------------------------------------------------------------------
TEST(MatchDelegateWorldCupTest,
     ProcessScoreUpdate_PendingGroupMatches_DoesNotCrash) {
    Fixture fx;

    ScoreUpdateEvent evt{};
    evt.tournamentId = "TID-3";

    auto m = std::make_shared<domain::Match>();
    m->TournamentId() = "TID-3";
    m->Round()        = "group";
    m->Home().Id()    = "H1";
    m->Home().Name()  = "Home 1";
    m->Visitor().Id() = "V1";
    m->Visitor().Name() = "Visitor 1";

    std::vector<std::shared_ptr<domain::Match>> matches{m};
    (void)matches;

    fx.delegate.ProcessScoreUpdate(evt);
}

// ---------------------------------------------------------------------
// ProcessScoreUpdate: all group matches played scenario (smoke test)
// ---------------------------------------------------------------------
TEST(MatchDelegateWorldCupTest,
     ProcessScoreUpdate_AllGroupMatchesPlayed_DoesNotCrash) {
    Fixture fx;

    ScoreUpdateEvent evt{};
    evt.tournamentId = "TID-4";

    std::vector<std::shared_ptr<domain::Match>> matches;
    auto tour = std::make_shared<domain::Tournament>("Knockout Only");
    tour->Id() = "TID-4";
    (void)matches;
    (void)tour;

    fx.delegate.ProcessScoreUpdate(evt);
}
