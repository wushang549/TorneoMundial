//
// Created by tomas on 9/10/25.
//

#ifndef TOURNAMENTS_MESSAGECONSUMER_HPP
#define TOURNAMENTS_MESSAGECONSUMER_HPP

#include <activemq/core/ActiveMQConnectionFactory.h>
#include <cms/Connection.h>
#include <cms/MessageConsumer.h>
#include <print>

class MessageProducer: public cms::MessageListener{
public:
    void SendMessage(const std::string_view& message) {
        std::shared_ptr<activemq::core::ActiveMQConnectionFactory> connectionFactory = std::make_shared<activemq::core::ActiveMQConnectionFactory>("failover://(tcp://localhost:61616)");
        std::shared_ptr<cms::Connection> connection = std::shared_ptr<cms::Connection>(connectionFactory->createConnection());
        connection->start();

        std::shared_ptr<cms::Session> session = std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
        std::shared_ptr<cms::Destination> destination = std::shared_ptr<cms::Destination>(session->createQueue("foo.bar"));
        auto consumer = std::shared_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));
        consumer->setMessageListener(this);
    }

    void onMessage(const cms::Message* message) override {
        const cms::TextMessage* textMessage = dynamic_cast<const cms::TextMessage*> (message);
        std::prinf(textMessage->getText());
    }
};

#endif //TOURNAMENTS_MESSAGECONSUMER_HPP