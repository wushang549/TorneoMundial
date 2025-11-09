#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "delegate/MatchDelegate.hpp"
#include "domain/Match.hpp"

#include "mocks/MatchRepositoryMock.hpp"
#include "mocks/TournamentDelegateMock.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

constexpr const char* kTid = "11111111-2222-3333-4444-555555555555";
constexpr const char* kMid = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";

// We avoid depending on Tournament constructor: return any non-null smart ptr.
inline std::shared_ptr<domain::Tournament> AnyTournamentPtr() {
    return std::shared_ptr<domain::Tournament>(
        reinterpret_cast<domain::Tournament*>(0x1),
        [](domain::Tournament*){} // no-op deleter
    );
}

static std::shared_ptr<domain::Match> makeMatch(
        std::string id, std::string tid, std::string round,
        std::string homeId, std::string homeName,
        std::string visId,  std::string visName,
        std::string status = "pending") {
    auto m = std::make_shared<domain::Match>();
    m->Id() = std::move(id);
    m->TournamentId() = std::move(tid);
    m->Round() = std::move(round);
    m->Home().Id() = std::move(homeId);
    m->Home().Name() = std::move(homeName);
    m->Visitor().Id() = std::move(visId);
    m->Visitor().Name() = std::move(visName);
    m->Status() = std::move(status);
    return m;
}

struct Fixture {
    std::shared_ptr<StrictMock<MatchRepositoryMock>> repo =
        std::make_shared<StrictMock<MatchRepositoryMock>>();
    std::shared_ptr<StrictMock<TournamentDelegateMock>> tdel =
        std::make_shared<StrictMock<TournamentDelegateMock>>();
    MatchDelegate delegate{repo, tdel};
};

} // namespace

// ReadAll
TEST(MatchDelegateTest, ReadAll_404_WhenTournamentMissing) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::unexpected(std::string{"not_found"})));

    EXPECT_THROW({ (void)fx.delegate.ReadAll(kTid, std::nullopt); }, std::runtime_error);
}

TEST(MatchDelegateTest, ReadAll_FilterPlayed) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>{AnyTournamentPtr()}));

    auto m1 = makeMatch("m1", kTid, "qf", "h1","H1","v1","V1","played");
    auto m2 = makeMatch("m2", kTid, "qf", "h2","H2","v2","V2","pending");

    EXPECT_CALL(*fx.repo, FindByTournamentId(kTid))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Match>>{m1, m2}));

    auto out = fx.delegate.ReadAll(kTid, std::optional<std::string_view>{"played"});
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0]->Id(), "m1");
}

// ReadById
TEST(MatchDelegateTest, ReadById_ReturnsRepoValue) {
    Fixture fx;
    auto m = makeMatch(kMid, kTid, "final", "H","Home","V","Visitor","pending");
    EXPECT_CALL(*fx.repo, FindByTournamentIdAndMatchId(kTid, kMid))
        .WillOnce(Return(m));

    auto r = fx.delegate.ReadById(kTid, kMid);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->Id(), kMid);
}

// UpdateScore
TEST(MatchDelegateTest, UpdateScore_NotFound) {
    Fixture fx;
    EXPECT_CALL(*fx.repo, FindByTournamentIdAndMatchId(kTid, kMid))
        .WillOnce(Return(nullptr));

    auto r = fx.delegate.UpdateScore(kTid, kMid, 1, 0);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "not_found");
}

TEST(MatchDelegateTest, UpdateScore_OutOfRange) {
    Fixture fx;
    auto r = fx.delegate.UpdateScore(kTid, kMid, 99, 0);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "validation:score_out_of_range");
}

TEST(MatchDelegateTest, UpdateScore_HomeWins_Persisted) {
    Fixture fx;
    auto m = makeMatch(kMid, kTid, "sf", "HID","Home","VID","Visitor","pending");
    EXPECT_CALL(*fx.repo, FindByTournamentIdAndMatchId(kTid, kMid))
        .WillOnce(Return(m));

    EXPECT_CALL(*fx.repo, Update(_))
        .WillOnce(Invoke([&](const domain::Match& updated) -> std::string {
            EXPECT_EQ(updated.Status(), "played");
            EXPECT_EQ(updated.WinnerTeamId().value(), "HID");
            EXPECT_EQ(updated.DecidedBy().value(), "regularTime");
            return updated.Id();
        }));

    auto r = fx.delegate.UpdateScore(kTid, kMid, 2, 1);
    EXPECT_TRUE(r.has_value());
}
// Create
TEST(MatchDelegateTest, Create_NotFoundTournament) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::unexpected(std::string{"not_found"})));

    nlohmann::json body = {
        {"round","qf"},
        {"home",    {{"id","11111111-1111-1111-1111-111111111111"},{"name","A"}}},
        {"visitor", {{"id","22222222-2222-2222-2222-222222222222"},{"name","B"}}}
    };
    auto r = fx.delegate.Create(kTid, body);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "not_found");
}

TEST(MatchDelegateTest, Create_ValidationErrors) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>{AnyTournamentPtr()}));

    nlohmann::json bad = {{"home", {{"id","x"},{"name","H"}}},
                          {"visitor", {{"id","y"},{"name","V"}}}};
    auto r1 = fx.delegate.Create(kTid, bad);
    ASSERT_FALSE(r1.has_value());
    EXPECT_TRUE(r1.error().rfind("validation:", 0) == 0);
}

TEST(MatchDelegateTest, Create_SameTeams) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>{AnyTournamentPtr()}));

    nlohmann::json body = {
        {"round","qf"},
        {"home",    {{"id","11111111-1111-1111-1111-111111111111"},{"name","A"}}},
        {"visitor", {{"id","11111111-1111-1111-1111-111111111111"},{"name","A"}}}
    };
    auto r = fx.delegate.Create(kTid, body);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), "validation:same_team_ids");
}

TEST(MatchDelegateTest, Create_Success_ReturnsId) {
    Fixture fx;
    EXPECT_CALL(*fx.tdel, ReadById(kTid))
        .WillOnce(Return(std::expected<std::shared_ptr<domain::Tournament>, std::string>{AnyTournamentPtr()}));

    nlohmann::json body = {
        {"round","final"},
        {"home",    {{"id","11111111-1111-1111-1111-111111111111"},{"name","A"}}},
        {"visitor", {{"id","22222222-2222-2222-2222-222222222222"},{"name","B"}}}
    };

    EXPECT_CALL(*fx.repo, Create(_))
        .WillOnce(Return(std::string{"new-id-xyz"}));

    auto r = fx.delegate.Create(kTid, body);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value(), "new-id-xyz");
}
