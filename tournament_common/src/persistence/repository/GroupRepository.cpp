//GroupRepository.cpp
// Created by root on 9/27/25.
//

#include "domain/Utilities.hpp"
#include "persistence/repository/GroupRepository.hpp"

#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <utility>

namespace {
std::shared_ptr<domain::Group> build_group_from_row(const pqxx::row& row) {
    const auto* documentPtr = row["document"].c_str();
    if (documentPtr == nullptr) {
        return nullptr;
    }

    const auto parsed = nlohmann::json::parse(documentPtr, nullptr, false);
    if (parsed.is_discarded()) {
        return nullptr;
    }

    domain::Group entity;
    parsed.get_to(entity);
    if (!row["id"].is_null()) {
        entity.Id() = row["id"].as<std::string>();
    }
    return std::make_shared<domain::Group>(entity);
}
} // namespace

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* conn = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(conn->connection));
    const pqxx::result result = tx.exec_params(
        "SELECT id, document FROM groups WHERE id = $1::uuid",
        id
    );
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    return build_group_from_row(result[0]);
}

std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, pqxx::params{entity.TournamentId(), groupBody.dump()});

    tx.commit();

    if (result.empty()) {
        return {};
    }

    return result[0]["id"].as<std::string>();
}

void GroupRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "DELETE FROM groups WHERE id = $1::uuid",
        id
    );
    tx.commit();
}


std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Group>> teams;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("select id, document->>'name' as name from groups")};
    tx.commit();

    for (const auto& row : result) {
        teams.push_back(std::make_shared<domain::Group>(domain::Group{row["name"].c_str(), row["id"].c_str()}));
    }

    return teams;
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::FindByTournamentId(const std::string_view& tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_groups_by_tournament"}, pqxx::params{tournamentId.data()});
    tx.commit();

    std::vector<std::shared_ptr<domain::Group>> groups;
    groups.reserve(result.size());
    for (const auto& row : result) {
        auto group = build_group_from_row(row);
        if (group) {
            groups.push_back(group);
        }
    }

    return groups;
}
// GroupRepository.cpp
std::string GroupRepository::Update(const domain::Group& entity) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    nlohmann::json body = entity; // usa tu to_json(Group)
    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "UPDATE groups "
        "SET document = $2::jsonb, last_update_date = CURRENT_TIMESTAMP "
        "WHERE id = $1::uuid "
        "RETURNING id",
        entity.Id(),                // $1
        body.dump()                 // $2
    );
    tx.commit();

    if (r.empty()) {
        return {};
    }

    return r[0]["id"].as<std::string>();
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_tournamentid_groupid"}, pqxx::params{tournamentId.data(), groupId.data()});
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    return build_group_from_row(result[0]);
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_group_in_tournament"}, pqxx::params{tournamentId.data(), teamId.data()});
    tx.commit();
    if (result.empty()) {
        return nullptr;
    }
    return build_group_from_row(result[0]);
}

void GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) {
    nlohmann::json teamDocument = team;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"update_group_add_team"}, pqxx::params{groupId.data(), teamDocument.dump()});
    tx.commit();
}