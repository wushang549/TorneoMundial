//
// QueueMessageListener.hpp
//
#ifndef COMMON_QUEUE_MESSAGE_CONSUMER_HPP
#define COMMON_QUEUE_MESSAGE_CONSUMER_HPP

#include <atomic>
#include <memory>
#include <thread>
#include <iostream>

#include <cms/Session.h>
#include <cms/Message.h>
#include <cms/MessageConsumer.h>
#include <cms/TextMessage.h>
#include <cms/Queue.h>
#include <cms/CMSException.h>

#include "cms/ConnectionManager.hpp"

class QueueMessageListener {
    std::shared_ptr<ConnectionManager> connectionManager;
    std::atomic<bool> running{false};
    // Note: Start() is blocking by design; we do not use an internal worker thread.
    std::shared_ptr<cms::Session> session;
    std::shared_ptr<cms::MessageConsumer> messageConsumer;

    virtual void processMessage(const std::string& message) = 0;

public:
    explicit QueueMessageListener(const std::shared_ptr<ConnectionManager>& connectionManager)
        : connectionManager(connectionManager) {}

    virtual ~QueueMessageListener() = default;

    void Start(const std::string_view& queueName);
    void Stop();
};

inline void QueueMessageListener::Start(const std::string_view& queueName) {
    if (running) return;
    running = true;

    try {
        session = connectionManager->CreateSession();

        auto destination = std::unique_ptr<cms::Queue>(
            session->createQueue(std::string(queueName).c_str())
        );
        // Keep the consumer as a member so Stop() can close it.
        messageConsumer.reset(session->createConsumer(destination.get()));

        while (running) {
            std::unique_ptr<cms::Message> message(messageConsumer->receive(1500));
            if (!message) continue;

            if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
                processMessage(text->getText());
            }
            // If needed, handle other message types here.
        }
    } catch (const cms::CMSException& e) {
        std::cerr << "[QueueMessageListener] CMSException: " << e.getMessage() << std::endl;
        running = false;
    } catch (const std::exception& e) {
        std::cerr << "[QueueMessageListener] std::exception: " << e.what() << std::endl;
        running = false;
    } catch (...) {
        std::cerr << "[QueueMessageListener] unknown exception\n";
        running = false;
    }
}

inline void QueueMessageListener::Stop() {
    running = false;

    // Close resources defensively; CMS allows closing in any order.
    try {
        if (messageConsumer) {
            try { messageConsumer->close(); } catch (...) {}
            messageConsumer.reset();
        }
        if (session) {
            try { session->close(); } catch (...) {}
            session.reset();
        }
    } catch (const cms::CMSException& e) {
        std::cerr << "[QueueMessageListener] Stop error: " << e.getMessage() << std::endl;
    }
}

#endif // COMMON_QUEUE_MESSAGE_CONSUMER_HPP
