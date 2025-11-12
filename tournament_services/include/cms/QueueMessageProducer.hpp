//
// Created by tomas on 9/9/25.
//

#ifndef SERVICE_MESSAGE_PRODUCER_HPP
#define SERVICE_MESSAGE_PRODUCER_HPP

#include <string_view>
#include <memory>

#include "IQueueMessageProducer.hpp"
#include "cms/ConnectionManager.hpp"

class QueueMessageProducer: public IQueueMessageProducer {
    std::shared_ptr<ConnectionManager> connectionManager;
public:
    explicit QueueMessageProducer(const std::shared_ptr<ConnectionManager>& connectionManager) : connectionManager(connectionManager){}

    // QueueMessageProducer.hpp  (solo método SendMessage)
    void SendMessage(const std::string_view& message, const std::string_view& queue) override {
        try {
            auto session = connectionManager->CreateSession(); // shared_ptr<cms::Session>

            std::unique_ptr<cms::Destination> dest(session->createQueue(queue.data()));
            std::unique_ptr<cms::MessageProducer> prod(session->createProducer(dest.get()));

            // Choose delivery mode (Persistent for durability; NonPersistent for speed)
            prod->setDeliveryMode(cms::DeliveryMode::PERSISTENT);

            std::unique_ptr<cms::TextMessage> msg(session->createTextMessage(std::string(message)));
            prod->send(msg.get());

            // Ensure clean shutdown order
            prod->close();
            session->close();
        } catch (const cms::CMSException& e) {
            // Minimal logging; adjust to your logger
            std::cerr << "[QueueMessageProducer] CMSException: " << e.getMessage() << std::endl;
            throw; // rethrow so caller can decide (optional)
        } catch (const std::exception& e) {
            std::cerr << "[QueueMessageProducer] Exception: " << e.what() << std::endl;
            throw;
        }
    }

};

#endif //SERVICE_MESSAGE_PRODUCER_HPP