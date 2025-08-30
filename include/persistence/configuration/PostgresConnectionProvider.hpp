//
// Created by tomas on 8/29/25.
//

#ifndef TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP
#define TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP
#include <condition_variable>
#include <queue>
#include <pqxx/pqxx>

#include "IDbConnectionProvider.hpp"

class PostgresConnectionProvider : public IDbConnectionProvider{
    std::string_view connectionString;
    size_t poolSize = 1;
    std::queue<std::unique_ptr<pqxx::connection>> connectionPool;
    std::mutex connectionPoolMutex;
    std::condition_variable connectionPoolCondition;

public:
    PostgresConnectionProvider(std::string_view connectionString, size_t poolSize) : connectionString(connectionString), poolSize(poolSize) {}

    std::unique_ptr<IDbConnection> Connection() override {
        std::unique_lock<std::mutex> lock(connectionPoolMutex);
        connectionPoolCondition.wait(lock, [this] { return !connectionPool.empty(); });

        auto connection = std::move(connectionPool.front());
        connectionPool.pop();

        return std::unique_ptr<IDbConnection>(
            new PostgresConnection(std::unique_ptr<pqxx::connection>(connection.release())),
            [this](IDbConnection* dbc) {
                auto pc = dynamic_cast<PostgresConnection*>(dbc);
                std::lock_guard<std::mutex> lock(connectionPoolMutex);
                connectionPool.push(std::move(pc->conn));
                delete pc;
                connectionPoolCondition.notify_one();
            });
    }
};
#endif //TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP