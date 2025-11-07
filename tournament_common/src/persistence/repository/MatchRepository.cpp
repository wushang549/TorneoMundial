#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"

using nlohmann::json;

// Escape helper for JSON string building (keeps it consistent with other repos).
static std::string esc(const std::string& s) {
    std::string out; out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\' || c == '\"') { out.push_back('\\'); out.push_back(c); }
        else out.push_back(c);
    }
    return out;
}

// Serialize full match "document" as a compact JSON string for the jsonb column.
// Only includes "score" if present, mirroring the API.
std::string MatchRepository::to_doc_string(const domain::Match& m) {
    // Build nested JSON manually to avoid incidental formatting differences
    // (same approach used in TournamentRepository).
    std::string doc = "{";

    doc += "\"tournamentId\":\""; doc += esc(m.TournamentId()); doc += "\",";
    doc += "\"round\":\"";        doc += esc(m.Round());        doc += "\",";

    doc += "\"home\":{";
    doc += "\"id\":\"";   doc += esc(m.Home().Id());   doc += "\",";
    doc += "\"name\":\""; doc += esc(m.Home().Name()); doc += "\"},";

    doc += "\"visitor\":{";
    doc += "\"id\":\"";   doc += esc(m.Visitor().Id());   doc += "\",";
    doc += "\"name\":\""; doc += esc(m.Visitor().Name()); doc += "\"},";

    doc += "\"status\":\""; doc += esc(m.Status()); doc += "\"";

    if (m.HasScore()) {
        doc += ",\"score\":{";
        doc += "\"home\":";    doc += std::to_string(*m.ScoreHome());    doc += ",";
        doc += "\"visitor\":"; doc += std::to_string(*m.ScoreVisitor());
        doc += "}";
    }
    if (m.WinnerTeamId().has_value()) {
        doc += ",\"winnerTeamId\":\""; doc += esc(*m.WinnerTeamId()); doc += "\"";
    }
    if (m.DecidedBy().has_value()) {
        doc += ",\"decidedBy\":\""; doc += esc(*m.DecidedBy()); doc += "\"";
    }
    if (m.NextMatchId().has_value()) {
        doc += ",\"nextMatchId\":\""; doc += esc(*m.NextMatchId()); doc += "\"";
    }
    if (m.NextMatchWinnerSlot().has_value()) {
        doc += ",\"nextMatchWinnerSlot\":\""; doc += esc(*m.NextMatchWinnerSlot()); doc += "\"";
    }

    doc += "}";
    return doc;
}

// Map row (id, document) -> domain::Match
std::shared_ptr<domain::Match> MatchRepository::row_to_domain(const pqxx::row& row) {
    const std::string id  = row["id"].c_str();
    json j = json::parse(row["document"].c_str());
    domain::Match m = j.get<domain::Match>();
    m.Id() = id;
    return std::make_shared<domain::Match>(std::move(m));
}

MatchRepository::MatchRepository(std::shared_ptr<IDbConnectionProvider> provider)
    : connectionProvider(std::move(provider)) {}

std::vector<std::shared_ptr<domain::Match>>
MatchRepository::FindByTournamentId(const std::string& tournamentId) {
    std::vector<std::shared_ptr<domain::Match>> out;

    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::read_transaction tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "SELECT id, document "
        "FROM matches "
        "WHERE tournament_id = $1::uuid "
        "ORDER BY created_at ASC",
        tournamentId
    );
    out.reserve(r.size());
    for (const auto& row : r) out.emplace_back(row_to_domain(row));
    return out;
}

std::shared_ptr<domain::Match>
MatchRepository::FindByTournamentIdAndMatchId(const std::string& tournamentId,
                                              const std::string& matchId) {
    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::read_transaction tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "SELECT id, document "
        "FROM matches "
        "WHERE tournament_id = $1::uuid AND id = $2::uuid "
        "LIMIT 1",
        tournamentId, matchId
    );
    if (r.empty()) return nullptr;
    return row_to_domain(r[0]);
}

std::string MatchRepository::Update(const domain::Match& entity) {
    if (entity.Id().empty()) throw std::invalid_argument("match.Id is required");
    if (entity.TournamentId().empty()) throw std::invalid_argument("match.TournamentId is required");

    auto pooled = connectionProvider->Connection();
    auto* conn  = dynamic_cast<PostgresConnection*>(&*pooled);

    const std::string doc = to_doc_string(entity);

    pqxx::work tx(*(conn->connection));
    pqxx::result r = tx.exec_params(
        "UPDATE matches "
        "SET document = $3::jsonb, last_update_date = CURRENT_TIMESTAMP "
        "WHERE tournament_id = $1::uuid AND id = $2::uuid",
        entity.TournamentId(), entity.Id(), doc
    );
    if (r.affected_rows() == 0) {
        tx.abort();
        throw std::runtime_error("not found");
    }
    tx.commit();
    return entity.Id();
}
