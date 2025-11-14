// GroupAddTeamListenerTest.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>

#include "cms/GroupAddTeamListener.hpp"
#include "mocks/MatchDelegateMock.hpp"

using ::testing::_;
using ::testing::StrictMock;

// Test helper to expose protected processMessage
class GroupAddTeamListenerTestable : public GroupAddTeamListener {
public:
    using GroupAddTeamListener::GroupAddTeamListener;
    using GroupAddTeamListener::processMessage;
};

TEST(GroupAddTeamListenerTest, ValidMessageCallsProcessTeamAdditionWithCorrectData) {
    std::shared_ptr<ConnectionManager> connMgr = nullptr;
    auto matchDelegate = std::make_shared<StrictMock<MatchDelegateMock>>();

    GroupAddTeamListenerTestable sut(connMgr, matchDelegate);

    const std::string message = R"({
        "tournamentId": "TID-123",
        "groupId":      "GID-456",
        "teamId":       "TEAM-789"
    })";

    EXPECT_CALL(*matchDelegate, ProcessTeamAddition(_))
        .WillOnce(::testing::Invoke([](const TeamAddEvent& evt) {
            EXPECT_EQ(evt.tournamentId, "TID-123");
            EXPECT_EQ(evt.groupId,      "GID-456");
            EXPECT_EQ(evt.teamId,       "TEAM-789");
        }));

    EXPECT_NO_THROW({
        sut.processMessage(message);
    });
}

TEST(GroupAddTeamListenerTest, DoesNothingWhenMatchDelegateIsNull) {
    std::shared_ptr<ConnectionManager> connMgr = nullptr;
    std::shared_ptr<MatchDelegate> nullDelegate = nullptr;

    GroupAddTeamListenerTestable sut(connMgr, nullDelegate);

    const std::string message = R"({
        "tournamentId": "TID",
        "groupId":      "GID",
        "teamId":       "TEAM"
    })";

    EXPECT_NO_THROW({
        sut.processMessage(message);
    });
}

TEST(GroupAddTeamListenerTest, InvalidJsonIsCaughtAndDoesNotThrow) {
    std::shared_ptr<ConnectionManager> connMgr = nullptr;
    auto matchDelegate = std::make_shared<StrictMock<MatchDelegateMock>>();

    GroupAddTeamListenerTestable sut(connMgr, matchDelegate);

    // Completely invalid JSON
    const std::string badMessage = R"(not-a-json)";

    EXPECT_NO_THROW({
        sut.processMessage(badMessage);
    });
}

TEST(GroupAddTeamListenerTest, MissingFieldsAreCaughtAndDoNotThrow) {
    std::shared_ptr<ConnectionManager> connMgr = nullptr;
    auto matchDelegate = std::make_shared<StrictMock<MatchDelegateMock>>();

    GroupAddTeamListenerTestable sut(connMgr, matchDelegate);

    // Valid JSON but missing required keys (groupId, teamId)
    const std::string badMessage = R"({
        "tournamentId": "TID-ONLY"
    })";

    EXPECT_NO_THROW({
        sut.processMessage(badMessage);
    });
}

TEST(GroupAddTeamListenerTest, DelegateExceptionIsCaughtAndDoesNotPropagate) {
    std::shared_ptr<ConnectionManager> connMgr = nullptr;
    auto matchDelegate = std::make_shared<StrictMock<MatchDelegateMock>>();

    GroupAddTeamListenerTestable sut(connMgr, matchDelegate);

    const std::string message = R"({
        "tournamentId": "TID-ERR",
        "groupId":      "GID-ERR",
        "teamId":       "TEAM-ERR"
    })";

    EXPECT_CALL(*matchDelegate, ProcessTeamAddition(_))
        .WillOnce(::testing::Invoke([](const TeamAddEvent&) {
            throw std::runtime_error("delegate boom");
        }));

    EXPECT_NO_THROW({
        sut.processMessage(message);
    });
}
