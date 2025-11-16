#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>

#define private public
#define protected public
#include "cms/ScoreUpdateListener.hpp"
#undef private
#undef protected

#include "mocks/MatchDelegateMock.hpp"

using ::testing::_;
using ::testing::StrictMock;

namespace {

struct Fixture {
    std::shared_ptr<StrictMock<MatchDelegateMock>> delegateMock =
        std::make_shared<StrictMock<MatchDelegateMock>>();

    std::shared_ptr<ConnectionManager> conn = nullptr;

    ScoreUpdateListener listener{conn, delegateMock};
};

} // namespace

// ----------------- Tests -----------------

// Mensaje válido -> debe llamar al delegate una vez
TEST(ScoreUpdateListenerTest, ValidJson_CallsDelegateOnce) {
    Fixture fx;

    const std::string payload =
        R"({"tournamentId":"TID-123","matchId":"MID-456"})";

    EXPECT_CALL(*fx.delegateMock, ProcessScoreUpdate(_))
        .Times(1);

    fx.listener.processMessage(payload);
}

// Falta tournamentId -> no debe llamar al delegate
TEST(ScoreUpdateListenerTest, MissingTournamentId_DoesNotCallDelegate) {
    Fixture fx;

    const std::string payload =
        R"({"matchId":"MID-456"})";

    EXPECT_CALL(*fx.delegateMock, ProcessScoreUpdate(_))
        .Times(0);

    fx.listener.processMessage(payload);
}

// Falta matchId -> no debe llamar al delegate
TEST(ScoreUpdateListenerTest, MissingMatchId_DoesNotCallDelegate) {
    Fixture fx;

    const std::string payload =
        R"({"tournamentId":"TID-123"})";

    EXPECT_CALL(*fx.delegateMock, ProcessScoreUpdate(_))
        .Times(0);

    fx.listener.processMessage(payload);
}

// JSON inválido -> se atrapa la excepción, no debe llamar al delegate
TEST(ScoreUpdateListenerTest, InvalidJson_DoesNotCallDelegate) {
    Fixture fx;

    const std::string payload = "not-a-json";

    EXPECT_CALL(*fx.delegateMock, ProcessScoreUpdate(_))
        .Times(0);

    fx.listener.processMessage(payload);
}

// matchDelegate == nullptr -> no debe explotar ni llamar nada
TEST(ScoreUpdateListenerTest, NullDelegate_DoesNotCrashAndDoesNotCall) {
    std::shared_ptr<MatchDelegate> nullDelegate = nullptr;
    std::shared_ptr<ConnectionManager> conn = nullptr;

    ScoreUpdateListener listener{conn, nullDelegate};

    const std::string payload =
        R"({"tournamentId":"TID-123","matchId":"MID-456"})";

    // Solo verificamos que no truene; no hay EXPECT_CALL porque no hay mock
    listener.processMessage(payload);
}
