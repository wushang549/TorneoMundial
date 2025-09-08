//
// Created by tomas on 9/5/25.
//
#include <thread>
#include <proton/container.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/connection.hpp>
#include <proton/sender.hpp>
#include <proton/message.hpp>
#include <proton/transport.hpp>

#include "amqp/MessagingProducer.hpp"

#include <iostream>
#include <mutex>

MessagingProducer::MessagingProducer(const std::string_view& url, const std::string_view& address): url(url), address(address) {
    containerThread = std::thread([this] {
        running = true;
        proton::container(*this).run();
    });
}

MessagingProducer::~MessagingProducer() {
    running = false;
    if (containerThread.joinable()) containerThread.join();
}

void MessagingProducer::SendMessage(const std::string_view& body) {
    {
        std::unique_lock lock(mtx);
        messages.push(std::string(body.data()));
    }
    cv.notify_one();
}

void MessagingProducer::on_container_start(proton::container& c) {
    c.connect(url.data());
}

void MessagingProducer::on_connection_open(proton::connection& conn) {
    conn.open_sender(address.data());
}

void MessagingProducer::on_sender_open(proton::sender& s) {
    std::unique_lock lock(mtx);
    sender = s;
    senderReady = true;
    cv.notify_one();
}

void MessagingProducer::on_sendable(proton::sender& s) {
    std::unique_lock lock(mtx);
    while (!messages.empty() && s.credit() > 0) {
        auto body = messages.front();
        messages.pop();

        proton::message msg;
        s.send(msg);
        std::cout << "[MessagingProducer] Sent: " << body << std::endl;
    }
}

void MessagingProducer::on_transport_error(proton::transport& t) {
    std::cerr << "Transport error: " << t.error().what() << std::endl;
}

void MessagingProducer::on_error(const proton::error_condition& ec) {
    std::cerr << "General error: " << ec.what() << std::endl;
}