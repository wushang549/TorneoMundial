#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>

#include <cms/Session.h>
#include "cms/ConnectionManager.hpp"
#include "cms/QueueMessageListener.hpp"
#include "mocks/ConnectionManagerMock.hpp"

using ::testing::NiceMock;
using ::testing::Return;
class TestQueueMessageListener : public QueueMessageListener {
public:
    explicit TestQueueMessageListener(const std::shared_ptr<ConnectionManager>& cm)
        : QueueMessageListener(cm) {}

    void processMessage(const std::string& message) override {
        lastMessage = message;
        processedCount++;
    }

    std::string lastMessage;
    int processedCount{0};
};

TEST(QueueMessageListenerTest, StopIsSafeWithoutStart) {
    auto cm = std::make_shared<NiceMock<ConnectionManagerMock>>();
    TestQueueMessageListener listener{cm};

    EXPECT_NO_THROW(listener.Stop());
    EXPECT_NO_THROW(listener.Stop());
}

TEST(QueueMessageListenerTest, DestructorIsSafeAfterStop) {
    auto cm = std::make_shared<NiceMock<ConnectionManagerMock>>();

    // Scope to force destructor execution
    {
        TestQueueMessageListener listener{cm};
        listener.Stop();
        // Destructor will be called at scope end; should not throw or crash
    }

    SUCCEED();
}

// This one does not call Start (so no CMS), just verifies processMessage wiring
TEST(QueueMessageListenerTest, ProcessMessageCanBeOverriddenInSubclass) {
    auto cm = std::make_shared<NiceMock<ConnectionManagerMock>>();
    TestQueueMessageListener listener{cm};

    const std::string payload = R"({"hello":"world"})";
    listener.processMessage(payload);

    EXPECT_EQ(listener.processedCount, 1);
    EXPECT_EQ(listener.lastMessage, payload);
}
