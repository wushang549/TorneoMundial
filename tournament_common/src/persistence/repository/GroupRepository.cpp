//
// Created by root on 9/27/25.
//

#include "domain/Utilities.hpp"
#include  "persistence/repository/GroupRepository.hpp"

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    return std::make_shared<domain::Group>();
}

std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, pqxx::params{entity.TournamentId(), groupBody.dump()});

    tx.commit();

    return result[0]["id"].c_str();
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

    for(auto row : result){
        teams.push_back(std::make_shared<domain::Group>(domain::Group{row["id"].c_str(), row["name"].c_str()}));
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
    for(auto row : result){
        nlohmann::json groupDocument = nlohmann::json::parse(row["document"].c_str());
        auto group = std::make_shared<domain::Group>(groupDocument);
        group->Id() = result[0]["id"].c_str();

        groups.push_back(group);
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
    if (r.empty()) { tx.abort(); throw std::runtime_error("update failed"); }
    const std::string id = r[0]["id"].as<std::string>();
    tx.commit();
    return id;
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_tournamentid_groupid"}, pqxx::params{tournamentId.data(), groupId.data()});
    tx.commit();
    nlohmann::json groupDocument = nlohmann::json::parse(result[0]["document"].c_str());
    auto group = std::make_shared<domain::Group>(groupDocument);
    group->Id() = result[0]["id"].c_str();

    return group;
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
    nlohmann::json groupDocument = nlohmann::json::parse(result[0]["document"].c_str());
    std::shared_ptr<domain::Group> group = std::make_shared<domain::Group>(groupDocument);
    group->Id() = result[0]["id"].c_str();

    return group;
}

void GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team> & team) {
    nlohmann::json teamDocument = team;
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"update_group_add_team"}, pqxx::params{groupId.data(), teamDocument.dump()});
    tx.commit();
}