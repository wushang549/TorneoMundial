//
// Created by tsuny on 9/1/25.
//
#include <memory>
#include <string>

#include "persistence/repository/TournamentRepository.hpp"

TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {

}

std::string TournamentRepository::Create (const domain::Tournament & entity) {
    return "id";
}

std::string TournamentRepository::Update (const domain::Tournament & entity) {
    return "id";
}

void TournamentRepository::Delete(std::string id) {

}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {

}