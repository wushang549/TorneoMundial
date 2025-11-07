#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"  // required by REGISTER_ROUTE(...)
#include "delegate/MatchDelegate.hpp"         // brings IMatchDelegate interface
#include <nlohmann/json.hpp>
#include <regex>
#include <optional>
#include <string_view>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

// If your project constant ID_VALUE is not resolving, use a local UUID pattern.
// This keeps the file standalone and avoids dependency issues.
static const std::regex UUID_RE("^[0-9a-fA-F]{8}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{12}$");

// GET /tournaments/{tId}/matches?showMatches=played|pending
crow::response MatchController::ReadAll(const crow::request& request,
                                        const std::string& tournamentId) const {
    // Path param validation (UUID expected)
    if (!std::regex_match(tournamentId, UUID_RE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    try {
        // Optional filter: "played" or "pending"
        std::optional<std::string_view> filter;
        if (const char* p = request.url_params.get("showMatches")) {
            filter = std::string_view{p};
        }

        // Delegate fetch
        auto matches = matchDelegate->ReadAll(tournamentId, filter);

        // Convert vector<shared_ptr<Match>> to JSON array (uses to_json(Match))
        nlohmann::json body = nlohmann::json::array();
        for (const auto& m : matches) {
            if (m) body.push_back(*m);
        }

        // Build HTTP response explicitly (avoid copy-ctor warnings)
        crow::response res(body.dump());
        res.code = crow::OK;
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    } catch (const std::runtime_error& e) {
        // Delegate throws "not_found" when tournament does not exist
        if (std::string_view{e.what()} == "not_found") {
            return crow::response{crow::NOT_FOUND, "tournament not found"};
        }
        return crow::response{crow::INTERNAL_SERVER_ERROR, "read matches failed"};
    } catch (...) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, "read matches failed"};
    }
}

// GET /tournaments/{tId}/matches/{mId}
crow::response MatchController::ReadById(const std::string& tournamentId,
                                         const std::string& matchId) const {
    if (!std::regex_match(tournamentId, UUID_RE) ||
        !std::regex_match(matchId,      UUID_RE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        auto m = matchDelegate->ReadById(tournamentId, matchId);
        if (!m) {
            return crow::response{crow::NOT_FOUND, "match not found"};
        }

        nlohmann::json body = *m;

        crow::response res(body.dump());
        res.code = crow::OK;
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    } catch (...) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, "read match failed"};
    }
}

// PATCH /tournaments/{tId}/matches/{mId}
crow::response MatchController::PatchScore(const crow::request& request,
                                           const std::string& tournamentId,
                                           const std::string& matchId) const {
    if (!std::regex_match(tournamentId, UUID_RE) ||
        !std::regex_match(matchId,      UUID_RE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }

    // Minimal payload validation
    auto body = nlohmann::json::parse(request.body);
    if (!body.contains("score") || !body["score"].is_object()) {
        return crow::response{crow::BAD_REQUEST, "Missing 'score' object"};
    }
    const auto& js = body["score"];
    if (!js.contains("home") || !js.contains("visitor") ||
        !js["home"].is_number_integer() || !js["visitor"].is_number_integer()) {
        return crow::response{crow::BAD_REQUEST, "Invalid score payload"};
    }
    const int home    = js["home"].get<int>();
    const int visitor = js["visitor"].get<int>();

    // Delegate applies rules: bounds check and no-draws with deterministic tiebreak
    auto r = matchDelegate->UpdateScore(tournamentId, matchId, home, visitor);
    if (!r) {
        const std::string err = r.error();
        if (err == "not_found") {
            return crow::response{crow::NOT_FOUND, "match not found"};
        }
        if (err.rfind("validation:", 0) == 0) {
            return crow::response{422, err.substr(std::string("validation:").size())};
        }
        return crow::response{crow::INTERNAL_SERVER_ERROR, "update score failed"};
    }

    return crow::response{crow::NO_CONTENT};
}

// Route bindings (keep them at global scope, after method definitions)
REGISTER_ROUTE(MatchController, ReadAll,    "/tournaments/<string>/matches",           "GET"_method)
REGISTER_ROUTE(MatchController, ReadById,   "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, PatchScore, "/tournaments/<string>/matches/<string>", "PATCH"_method)
