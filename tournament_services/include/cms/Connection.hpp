//
// Created by tomas on 9/10/25.
//

#ifndef TOURNAMENTS_CONNECTION_HPP
#define TOURNAMENTS_CONNECTION_HPP

#include <memory>
#include <activemq/core/ActiveMQConnectionFactory.h>

class Connection {
    std::shared_ptr<activemq::core::ActiveMQConnectionFactory> connectionFactory;

public:
    explicit Connection(std::shared_ptr<activemq::core::ActiveMQConnectionFactory> factory) : connectionFactory(std::move(factory)){}

    [[nodiscard]] std::shared_ptr<cms::MessageProducer> GetQueueProducer(const std::string_view& queueName) const {
        const auto connection = std::shared_ptr<cms::Connection>(connectionFactory->createConnection());
        connection->start();
        const auto session = std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
        const auto destination = std::shared_ptr<cms::Destination>(session->createQueue(queueName.data()));
        return std::shared_ptr<cms::MessageProducer>(session->createProducer(destination.get()));
    }
};
#endif //TOURNAMENTS_CONNECTION_HPP