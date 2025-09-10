//
// Created by tomas on 9/9/25.
//

#ifndef TOURNAMENTS_MESSAGEPRODUCER_HPP
#define TOURNAMENTS_MESSAGEPRODUCER_HPP

#include <string_view>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <cms/Connection.h>

class MessageProducer {
    public:
    void SendMessage(const std::string_view& message) {
        std::shared_ptr<activemq::core::ActiveMQConnectionFactory> connectionFactory = std::make_shared<activemq::core::ActiveMQConnectionFactory>("failover://(tcp://localhost:61616)");
        std::shared_ptr<cms::Connection> connection = std::shared_ptr<cms::Connection>(connectionFactory->createConnection());
        connection->start();

        std::shared_ptr<cms::Session> session = std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
        std::shared_ptr<cms::Destination> destination = std::shared_ptr<cms::Destination>(session->createQueue("foo.bar"));
        auto producer = std::shared_ptr<cms::MessageProducer>(session->createProducer(destination.get()));
        producer->setDeliveryMode( cms::DeliveryMode::NON_PERSISTENT );
        {
            std::unique_ptr<cms::TextMessage> brokerMessage = std::unique_ptr<cms::TextMessage>(session->createTextMessage("test message"));
            producer->send(brokerMessage.get());
        }
    }
};

#endif //TOURNAMENTS_MESSAGEPRODUCER_HPP