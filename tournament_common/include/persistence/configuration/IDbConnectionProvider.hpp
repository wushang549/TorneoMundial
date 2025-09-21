//
// Created by tomas on 8/29/25.
//

#ifndef TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP
#define TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP

#include <memory>
#include <functional>

class IDbConnection {
public:
    virtual ~IDbConnection() = default;
};


class PooledConnection {
    std::unique_ptr<IDbConnection, std::function<void(IDbConnection*)>> connection;
public:
    explicit PooledConnection(
        IDbConnection* dbc,
        std::function<void(IDbConnection*)> deleter) : connection(dbc, std::move(deleter)) {}

    IDbConnection* operator->() { return connection.get(); }
    IDbConnection& operator*() { return *connection; }
       // disable copy
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    // allow move
    PooledConnection(PooledConnection&&) noexcept = default;
    PooledConnection& operator=(PooledConnection&&) noexcept = default;
};


class IDbConnectionProvider {
public:
    virtual ~IDbConnectionProvider() = default;
    virtual PooledConnection Connection() = 0;
};
#endif //TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP