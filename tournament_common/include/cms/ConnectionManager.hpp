#pragma once
#ifndef SERVICES_CONNECTION_MANAGER_HPP
#define SERVICES_CONNECTION_MANAGER_HPP

#include <activemq/core/ActiveMQConnectionFactory.h>
#include <cms/Connection.h>
#include <cms/Session.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

class ConnectionManager {
public:
    // Call once from DI .onActivated(...)
    void initialize(std::string_view brokerURI,
                    std::string_view username = {},
                    std::string_view password = {},
                    std::string_view clientId = {}) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (connection_) {
            std::cout << "[ConnectionManager] Already initialized\n";
            return;
        }

        std::cout << "[ConnectionManager] Connecting to broker: " << brokerURI << std::endl;

        factory_ = std::make_unique<activemq::core::ActiveMQConnectionFactory>(std::string(brokerURI));

        // Create connection (with or without credentials)
        if (!username.empty() || !password.empty()) {
            connection_.reset(factory_->createConnection(std::string(username), std::string(password)));
        } else {
            connection_.reset(factory_->createConnection());
        }

        if (!clientId.empty()) {
            connection_->setClientID(std::string(clientId));
        }

        // IMPORTANT: start connection before creating sessions/consumers
        connection_->start();
        std::cout << "[ConnectionManager] Connection started\n";
    }

    // Optional explicit stop (DI container puede llamar esto en shutdown)
    void shutdown() {
        std::lock_guard<std::mutex> lock(mtx_);
        try {
            if (connection_) {
                std::cout << "[ConnectionManager] Stopping connection\n";
                // Close is enough; sessions/producers/consumers should be closed by owners.
                connection_->close();
                connection_.reset();
            }
            factory_.reset();
        } catch (const std::exception& e) {
            std::cout << "[ConnectionManager] shutdown() error: " << e.what() << std::endl;
        }
    }

    // Accessors
    [[nodiscard]] std::shared_ptr<cms::Connection> Connection() const {
        return connection_;
    }

    // Create a fresh AUTO_ACK session for consumers/producers
    [[nodiscard]] std::shared_ptr<cms::Session> CreateSession() const {
        auto conn = connection_;
        if (!conn) {
            throw std::runtime_error("[ConnectionManager] CreateSession(): connection not initialized");
        }
        // Each session must be closed by the caller (your listener does Stop()).
        return std::shared_ptr<cms::Session>(
            conn->createSession(cms::Session::AUTO_ACKNOWLEDGE));
    }

    ~ConnectionManager() {
        // best-effort shutdown
        shutdown();
    }

private:
    mutable std::mutex mtx_;
    std::unique_ptr<activemq::core::ActiveMQConnectionFactory> factory_;
    std::shared_ptr<cms::Connection> connection_;
};

#endif // SERVICES_CONNECTION_MANAGER_HPP
