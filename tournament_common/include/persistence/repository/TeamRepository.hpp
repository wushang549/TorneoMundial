//
// Created by tomas on 8/24/25.
//
#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP

#include <string>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"

class TeamRepository : public IRepository<domain::Team, std::string_view> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider)
        : connectionProvider(std::move(connectionProvider)) {}

    // READ ALL: return full document; Id (UUID) set from column
    std::vector<std::shared_ptr<domain::Team>> ReadAll() override {
        std::vector<std::shared_ptr<domain::Team>> teams;

        auto pooled = connectionProvider->Connection();
        auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::read_transaction tx{*(connection->connection)};
        // Fetch full JSON document so controllers can serialize everything
        pqxx::result result = tx.exec(
            "SELECT id, document FROM teams ORDER BY created_at ASC"
        );

        teams.reserve(result.size());
        for (const auto& row : result) {
            auto doc  = nlohmann::json::parse(row["document"].c_str());
            auto team = std::make_shared<domain::Team>(doc); // uses your Team(json) ctor
            team->Id  = row["id"].c_str();                   // set DB uuid
            teams.emplace_back(std::move(team));
        }
        return teams;
    }

    // READ BY ID (UUID). Return nullptr if not found.
    std::shared_ptr<domain::Team> ReadById(std::string_view id) override {
        auto pooled = connectionProvider->Connection();
        auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

        const std::string key{id}; // ensure type matches pqxx binding
        pqxx::read_transaction tx{*(connection->connection)};

        pqxx::result result = tx.exec_params(
            "SELECT id, document FROM teams WHERE id = $1::uuid LIMIT 1",
            key
        );
        if (result.empty()) {
            return nullptr;
        }

        auto doc  = nlohmann::json::parse(result[0]["document"].c_str());
        auto team = std::make_shared<domain::Team>(doc);
        team->Id  = result[0]["id"].c_str();
        return team;
    }

    // CREATE: insert JSON document; DB generates UUID; return it.
    std::string_view Create(const domain::Team &entity) override {
        auto pooled = connectionProvider->Connection();
        auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

        nlohmann::json body = entity; // rely on your to_json mapping

        pqxx::work tx{*(connection->connection)};
        pqxx::result result = tx.exec_params(
            "INSERT INTO teams (document) VALUES ($1::jsonb) RETURNING id",
            body.dump()
        );
        if (result.empty()) {
            tx.abort();
            throw std::runtime_error("insert failed");
        }
        tx.commit();

        // Return a string_view with stable storage
        thread_local std::string id_buffer;
        id_buffer = result[0]["id"].c_str();
        return std::string_view{id_buffer};
    }

    // UPDATE: set JSON document by UUID and update timestamp; return same id.
    std::string_view Update(const domain::Team &entity) override {
        if (entity.Id.empty()) {
            throw std::invalid_argument("team.Id is required for update");
        }
        auto pooled = connectionProvider->Connection();
        auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

        nlohmann::json body = entity;

        pqxx::work tx{*(connection->connection)};
        pqxx::result r = tx.exec_params(
            "UPDATE teams "
            "SET document = $2::jsonb, last_update_date = CURRENT_TIMESTAMP "
            "WHERE id = $1::uuid",
            entity.Id,
            body.dump()
        );
        if (r.affected_rows() == 0) {
            tx.abort();
            throw std::runtime_error("not found");
        }
        tx.commit();

        thread_local std::string id_buffer;
        id_buffer = entity.Id;
        return std::string_view{id_buffer};
    }

    // DELETE by UUID. Throws if not found.
    void Delete(std::string_view id) override {
        auto pooled = connectionProvider->Connection();
        auto* connection = dynamic_cast<PostgresConnection*>(&*pooled);

        const std::string key{id};

        pqxx::work tx{*(connection->connection)};
        pqxx::result r = tx.exec_params(
            "DELETE FROM teams WHERE id = $1::uuid",
            key
        );
        if (r.affected_rows() == 0) {
            tx.abort();
            throw std::runtime_error("not found");
        }
        tx.commit();
    }
};

#endif //RESTAPI_TEAMREPOSITORY_HPP
