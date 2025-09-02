//
// Created by tsuny on 9/1/25.
//
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"


TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {
    return std::make_shared<domain::Tournament>(domain::Tournament());
}

std::string TournamentRepository::Create (const domain::Tournament & entity) {

    nlohmann::json tournamentDoc = entity;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    connection->connection->prepare("insert_tournament", "insert into TOURNAMENTS (document) values($1) RETURNING id");

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentDoc.dump());

    tx.commit();

    return result[0]["id"].c_str();
}

std::string TournamentRepository::Update (const domain::Tournament & entity) {
    return "id";
}

void TournamentRepository::Delete(std::string id) {

}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("select id, document from tournaments")};
    tx.commit();

    for(auto row : result){
        nlohmann::json rowTournament = nlohmann::json::parse(row["document"].c_str());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = row["id"].c_str();

        tournaments.push_back(tournament);
    }

    return tournaments;
}