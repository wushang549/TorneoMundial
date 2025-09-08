//
// Created by tomas on 9/5/25.
//

#ifndef TOURNAMENTS_MESSAGING_PRODUCER_HPP
#define TOURNAMENTS_MESSAGING_PRODUCER_HPP

#include <condition_variable>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <proton/messaging_handler.hpp>
#include <proton/sender.hpp>


class MessagingProducer : public proton::messaging_handler {
    std::string url;
    std::string address;
    proton::sender sender;

    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::string> messages;
    bool senderReady = false;
    bool running = true;

    std::thread containerThread;
public:
    MessagingProducer(const std::string_view& url, const std::string_view& address);
    ~MessagingProducer();
    void SendMessage(const std::string_view& body);
    void on_container_start(proton::container& c) override;
    void on_connection_open(proton::connection& conn) override;
    void on_sender_open(proton::sender& s) override;
    void on_sendable(proton::sender& s) override;
    void on_transport_error(proton::transport& t) override;
    void on_error(const proton::error_condition& ec) override;
};

#endif //TOURNAMENTS_MESSAGING_PRODUCER_HPP