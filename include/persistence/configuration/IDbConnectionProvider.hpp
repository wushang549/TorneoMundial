//
// Created by tomas on 8/29/25.
//

#ifndef TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP
#define TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP
#include <memory>

#include <memory>

class IDbConnection {
public:
    virtual ~IDbConnection() = default;
};

class IDbConnectionProvider {
public:
    virtual ~IDbConnectionProvider() = default;
    virtual std::unique_ptr<IDbConnection> Connection() = 0;
};
#endif //TOURNAMENTS_IDBCONNECTIONPROVIDER_HPP