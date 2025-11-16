#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdlib>
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

#include "crow.h"
#include <nlohmann/json.hpp>

#include "controller/MatchController.hpp"
#include "domain/Match.hpp"

#include "mocks/MatchDelegateMock.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

constexpr const char* kValidTid  = "11111111-2222-3333-4444-555555555555";
constexpr const char* kValidMid  = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";
constexpr const char* kInvalidId = "not-a-uuid";

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
    Fixture() {
        // Disable score publish for all MatchController tests
        ::setenv("DISABLE_SCORE_PUBLISH", "1", 1);
    }

    std::shared_ptr<StrictMock<MatchDelegateMock>> mock =
        std::make_shared<StrictMock<MatchDelegateMock>>();
    MatchController controller{mock};
};


} // namespace

// ---------- ReadAll ----------

TEST(MatchControllerTest, ReadAll_InvalidTournamentId_400) {
    Fixture fx;
    crow::request req;
    auto res = fx.controller.ReadAll(req, kInvalidId);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, ReadAll_Success_200_Array) {
    Fixture fx;
    auto m1 = makeMatch("m1", kValidTid, "qf", "h1","H1","v1","V1");
    auto m2 = makeMatch("m2", kValidTid, "sf", "h2","H2","v2","V2","played");

    EXPECT_CALL(*fx.mock, ReadAll(kValidTid, _))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Match>>{m1, m2}));

    crow::request req;
    auto res = fx.controller.ReadAll(req, kValidTid);
    EXPECT_EQ(res.code, crow::OK);

    auto j = nlohmann::json::parse(res.body);
    ASSERT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2);
    EXPECT_EQ(j[0]["round"], "qf");
    EXPECT_EQ(j[1]["status"], "played");

    auto ct = res.get_header_value("content-type");
    EXPECT_EQ(ct, "application/json");
}

TEST(MatchControllerTest, ReadAll_DelegateNotFound_404) {
    Fixture fx;
    crow::request req;

    EXPECT_CALL(*fx.mock, ReadAll(kValidTid, _))
        .WillOnce(Invoke([](const std::string&,
                            std::optional<std::string_view>) -> std::vector<std::shared_ptr<domain::Match>> {
            throw std::runtime_error("not_found");
        }));

    auto res = fx.controller.ReadAll(req, kValidTid);
    EXPECT_EQ(res.code, crow::NOT_FOUND);
    EXPECT_EQ(res.body, "tournament not found");
}

TEST(MatchControllerTest, ReadAll_DelegateRuntimeOther_500) {
    Fixture fx;
    crow::request req;

    EXPECT_CALL(*fx.mock, ReadAll(kValidTid, _))
        .WillOnce(Invoke([](const std::string&,
                            std::optional<std::string_view>) -> std::vector<std::shared_ptr<domain::Match>> {
            throw std::runtime_error("db_error");
        }));

    auto res = fx.controller.ReadAll(req, kValidTid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "read matches failed");
}

TEST(MatchControllerTest, ReadAll_DelegateUnknownException_500) {
    Fixture fx;
    crow::request req;

    EXPECT_CALL(*fx.mock, ReadAll(kValidTid, _))
        .WillOnce(Invoke([](const std::string&,
                            std::optional<std::string_view>) -> std::vector<std::shared_ptr<domain::Match>> {
            throw std::logic_error("logic");
        }));

    auto res = fx.controller.ReadAll(req, kValidTid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "read matches failed");
}

// ---------- ReadById ----------

TEST(MatchControllerTest, ReadById_BadIds_400) {
    Fixture fx;
    auto res = fx.controller.ReadById(kInvalidId, kInvalidId);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, ReadById_NotFound_404) {
    Fixture fx;
    EXPECT_CALL(*fx.mock, ReadById(kValidTid, kValidMid))
        .WillOnce(Return(nullptr));

    auto res = fx.controller.ReadById(kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(MatchControllerTest, ReadById_Success_200) {
    Fixture fx;
    auto m = makeMatch(kValidMid, kValidTid, "final", "H","Home","V","Visitor","played");
    m->SetScore(2,1);
    m->SetWinnerTeamId("H");
    m->SetDecidedBy("regularTime");

    EXPECT_CALL(*fx.mock, ReadById(kValidTid, kValidMid))
        .WillOnce(Return(m));

    auto res = fx.controller.ReadById(kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::OK);

    auto j = nlohmann::json::parse(res.body);
    EXPECT_EQ(j["id"], kValidMid);
    EXPECT_EQ(j["status"], "played");
    EXPECT_EQ(j["winnerTeamId"], "H");

    auto ct = res.get_header_value("content-type");
    EXPECT_EQ(ct, "application/json");
}

TEST(MatchControllerTest, ReadById_DelegateThrows_500) {
    Fixture fx;

    EXPECT_CALL(*fx.mock, ReadById(kValidTid, kValidMid))
        .WillOnce(Invoke([](const std::string&, const std::string&)
                         -> std::shared_ptr<domain::Match> {
            throw std::runtime_error("boom");
        }));

    auto res = fx.controller.ReadById(kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "read match failed");
}

// ---------- PatchScore ----------

TEST(MatchControllerTest, PatchScore_BadIds_400) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":{"home":2,"visitor":1}})";
    auto res = fx.controller.PatchScore(req, kInvalidId, kValidMid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, PatchScore_BadJson_400) {
    Fixture fx;
    crow::request req;
    req.body = "{ invalid";
    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, PatchScore_MissingScore_400) {
    Fixture fx;
    crow::request req;
    req.body = R"({"nope":1})";
    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
    EXPECT_EQ(res.body, "Missing 'score' object");
}

TEST(MatchControllerTest, PatchScore_ScoreNotObject_400) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":123})";
    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
    EXPECT_EQ(res.body, "Missing 'score' object");
}

TEST(MatchControllerTest, PatchScore_InvalidScorePayload_400) {
    Fixture fx;
    crow::request req;
    // home is not integer
    req.body = R"({"score":{"home":"x","visitor":1}})";
    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
    EXPECT_EQ(res.body, "Invalid score payload");
}

TEST(MatchControllerTest, PatchScore_DelegateNotFound_404) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":{"home":1,"visitor":0}})";

    EXPECT_CALL(*fx.mock, UpdateScore(kValidTid, kValidMid, 1, 0))
        .WillOnce(Return(std::unexpected(std::string("not_found"))));

    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(MatchControllerTest, PatchScore_Validation_422) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":{"home":11,"visitor":0}})";

    EXPECT_CALL(*fx.mock, UpdateScore(kValidTid, kValidMid, 11, 0))
        .WillOnce(Return(std::unexpected(std::string("validation:score_out_of_range"))));

    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, 422);
}

TEST(MatchControllerTest, PatchScore_UnexpectedError_500) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":{"home":2,"visitor":1}})";

    EXPECT_CALL(*fx.mock, UpdateScore(kValidTid, kValidMid, 2, 1))
        .WillOnce(Return(std::unexpected(std::string("db_error"))));

    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "update score failed");
}

TEST(MatchControllerTest, PatchScore_Success_204) {
    Fixture fx;
    crow::request req;
    req.body = R"({"score":{"home":2,"visitor":1}})";

    EXPECT_CALL(*fx.mock, UpdateScore(kValidTid, kValidMid, 2, 1))
        .WillOnce(Invoke([](const std::string&,
                            const std::string&,
                            int /*home*/,
                            int /*visitor*/) {
            // Success: std::expected<void, std::string> in value state
            return std::expected<void, std::string>{std::in_place};
        }));

    auto res = fx.controller.PatchScore(req, kValidTid, kValidMid);
    EXPECT_EQ(res.code, crow::NO_CONTENT);
}


// ---------- Create ----------

TEST(MatchControllerTest, Create_BadTid_400) {
    Fixture fx;
    crow::request req;
    req.body = R"({"round":"qf","home":{"id":"h","name":"H"},"visitor":{"id":"v","name":"V"}})";
    auto res = fx.controller.Create(req, kInvalidId);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, Create_BadJson_400) {
    Fixture fx;
    crow::request req;
    req.body = "{not json";
    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST(MatchControllerTest, Create_Validation_422) {
    Fixture fx;
    crow::request req;
    req.body = R"({"round":"qf","home":{"id":"h","name":"H"},"visitor":{"id":"v"}})";

    EXPECT_CALL(*fx.mock, Create(kValidTid, _))
        .WillOnce(Return(std::unexpected(std::string("validation:missing_or_invalid_visitor"))));

    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, 422);
}

TEST(MatchControllerTest, Create_NotFound_404) {
    Fixture fx;
    crow::request req;
    req.body = R"({"round":"qf","home":{"id":"h","name":"H"},"visitor":{"id":"v","name":"V"}})";

    EXPECT_CALL(*fx.mock, Create(kValidTid, _))
        .WillOnce(Return(std::unexpected(std::string("not_found"))));

    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(MatchControllerTest, Create_UnexpectedError_500) {
    Fixture fx;
    crow::request req;
    req.body = R"({
      "round":"qf",
      "home":    {"id":"h","name":"H"},
      "visitor": {"id":"v","name":"V"}
    })";

    EXPECT_CALL(*fx.mock, Create(kValidTid, _))
        .WillOnce(Return(std::unexpected(std::string("db_error"))));

    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "create match failed");
}

TEST(MatchControllerTest, Create_DelegateThrows_500) {
    Fixture fx;
    crow::request req;
    req.body = R"({
      "round":"qf",
      "home":    {"id":"h","name":"H"},
      "visitor": {"id":"v","name":"V"}
    })";

    EXPECT_CALL(*fx.mock, Create(kValidTid, _))
        .WillOnce(Invoke([](const std::string&,
                            const nlohmann::json&) -> std::expected<std::string, std::string> {
            throw std::runtime_error("boom");
        }));

    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "create match failed");
}

TEST(MatchControllerTest, Create_Success_201) {
    Fixture fx;
    crow::request req;
    req.body = R"({
      "round":"final",
      "home":    {"id":"11111111-1111-1111-1111-111111111111","name":"A"},
      "visitor": {"id":"22222222-2222-2222-2222-222222222222","name":"B"}
    })";

    EXPECT_CALL(*fx.mock, Create(kValidTid, _))
        .WillOnce(Return(std::expected<std::string, std::string>{"new-id-123"}));

    auto res = fx.controller.Create(req, kValidTid);
    EXPECT_EQ(res.code, crow::CREATED);

    auto j = nlohmann::json::parse(res.body);
    EXPECT_EQ(j["id"], "new-id-123");

    auto ct = res.get_header_value("content-type");
    EXPECT_EQ(ct, "application/json");

    auto location = res.get_header_value("Location");
    EXPECT_EQ(location,
              std::string("/tournaments/") + kValidTid + "/matches/new-id-123");
}
