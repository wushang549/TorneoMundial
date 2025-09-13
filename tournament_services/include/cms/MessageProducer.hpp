//
// Created by tomas on 9/9/25.
//

#ifndef TOURNAMENTS_MESSAGEPRODUCER_HPP
#define TOURNAMENTS_MESSAGEPRODUCER_HPP

#include <string_view>
#include <memory>

#include "IMessageProducer.hpp"

class QueueMessageProducer: public IMessageProducer {
    std::shared_ptr<cms::MessageProducer> producer;
    std::shared_ptr<cms::Session> session;
    public:
    QueueMessageProducer(std::shared_ptr<cms::MessageProducer> producer, std::shared_ptr<cms::Session> session) : producer(std::move(producer)), session(std::move(session)) {

    }
    void SendMessage(const std::string_view& message) override {
        {
            std::unique_ptr<cms::TextMessage> brokerMessage = std::unique_ptr<cms::TextMessage>(session->createTextMessage(std::format("\"tournamentID:\": \"{}\"",message.data())));
            producer->send(brokerMessage.get());
        }
    }
};

#endif //TOURNAMENTS_MESSAGEPRODUCER_HPP