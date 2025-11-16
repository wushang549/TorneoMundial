#pragma once
#include <gmock/gmock.h>
#include "../include/cms/IQueueMessageProducer.hpp"

class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void,
                SendMessage,
                (const std::string_view& message, const std::string_view& queue),
                (override));
};
