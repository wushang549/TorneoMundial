//
// Created by tomas on 8/29/25.
//

#ifndef TOURNAMENTS_POSTGRES_CONNECTION_HPP
#define TOURNAMENTS_POSTGRES_CONNECTION_HPP
#include <memory>
#include <pqxx/pqxx>
#include "IDbConnectionProvider.hpp"


struct PostgresConnection final : IDbConnection{
    std::unique_ptr<pqxx::connection> connection;
    explicit PostgresConnection(std::unique_ptr<pqxx::connection> connection) : connection(std::move(connection)) {}
};



#endif //TOURNAMENTS_POSTGRESCONNECTIONPROVIDER_HPP