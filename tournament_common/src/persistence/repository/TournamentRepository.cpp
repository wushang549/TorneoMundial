#include <memory>
#include <string>
#include <stdexcept>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Tournament.hpp"

using nlohmann::json;

// enum <-> string helpers
static std::string type_to_string(domain::TournamentType t) {
    return (t == domain::TournamentType::NFL) ? "NFL" : "ROUND_ROBIN";
}
static domain::TournamentType string_to_type(const std::string& s) {
    return (s == "NFL") ? domain::TournamentType::NFL : domain::TournamentType::ROUND_ROBIN;
}

// simple escaper
static std::string esc(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\' || c == '\"') { out.push_back('\\'); out.push_back(c); }
        else out.push_back(c);
    }
    return out;
}

// domain -> JSON string for document column
static std::string to_doc_string(const domain::Tournament& t) {
    std::string doc = "{\"name\":\"";
    doc += esc(t.Name());
    doc += "\",\"format\":{";
    doc += "\"numberOfGroups\":";   doc += std::to_string(t.Format().NumberOfGroups());
    doc += ",\"maxTeamsPerGroup\":"; doc += std::to_string(t.Format().MaxTeamsPerGroup());
    doc += ",\"type\":\"";          doc += type_to_string(t.Format().Type()); doc += "\"}}";
    return doc;
}

// row -> domain
static std::shared_ptr<domain::Tournament> row_to_domain(const pqxx::row& row) {
    json j = json::parse(row["document"].c_str());
    const std::string name = j.value("name", "");
    const json& jf        = j.at("format");
    const int ng         = jf.value("numberOfGroups", 1);
    const int mtg        = jf.value("maxTeamsPerGroup", 16);
    const std::string ts = jf.value("type", "ROUND_ROBIN");

    domain::TournamentFormat fmt{ng, mtg, string_to_type(ts)};
    auto t = std::make_shared<domain::Tournament>(name, fmt);
    t->Id() = row["id"].as<std::string>();
    return t;
}

TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> provider)
    : connectionProvider(std::move(provider)) {}

std::string TournamentRepository::Create(const domain::Tournament& entity) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    const std::string doc = to_doc_string(entity);

    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "INSERT INTO tournaments (document) VALUES ($1::jsonb) RETURNING id",
        doc
    );
    if (r.empty()) { tx.abort(); throw std::runtime_error("insert failed"); }
    const std::string id = r[0]["id"].as<std::string>();
    tx.commit();
    return id;
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> out;

    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::read_transaction tx(*(conn->connection));
    pqxx::result r = tx.exec("SELECT id, document FROM tournaments ORDER BY created_at ASC");

    out.reserve(r.size());
    for (const auto& row : r) out.emplace_back(row_to_domain(row));
    return out;
}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::read_transaction tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "SELECT id, document FROM tournaments WHERE id = $1::uuid LIMIT 1",
        id
    );
    if (r.empty()) return nullptr;
    return row_to_domain(r[0]);
}

std::string TournamentRepository::Update(const domain::Tournament& entity) {
    if (entity.Id().empty()) throw std::invalid_argument("tournament.Id is required");

    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    const std::string doc = to_doc_string(entity);

    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "UPDATE tournaments "
        "SET document = $2::jsonb, last_update_date = CURRENT_TIMESTAMP "
        "WHERE id = $1::uuid",
        entity.Id(), doc
    );
    if (r.affected_rows() == 0) { tx.abort(); throw std::runtime_error("not found"); }
    tx.commit();
    return entity.Id();
}

void TournamentRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "DELETE FROM tournaments WHERE id = $1::uuid",
        id
    );
    if (r.affected_rows() == 0) { tx.abort(); throw std::runtime_error("not found"); }
    tx.commit();
}
