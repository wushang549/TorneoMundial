// MatchController.cpp
#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"
#include "delegate/MatchDelegate.hpp"

#include <nlohmann/json.hpp>
#include <regex>
#include <optional>
#include <string_view>
#include <cstdlib>   // std::getenv
#include <memory>
#include <iostream>

// ActiveMQ-CPP / CMS
#include <cms/CMSException.h>
#include <cms/Connection.h>
#include <cms/ConnectionFactory.h>
#include <cms/MessageProducer.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>

#define JSON_CONTENT_TYPE   "application/json"
#define CONTENT_TYPE_HEADER "content-type"

// Local UUID regex (keep file standalone)
static const std::regex UUID_RE("^[0-9a-fA-F]{8}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{4}-"
                                "[0-9a-fA-F]{12}$");

// Optional guard so tests can disable publishing
static bool is_score_publish_disabled() {
    if (const char* env = std::getenv("DISABLE_SCORE_PUBLISH")) {
        return env[0] != '\0';
    }
    return false;
}

// ----------- Simple one-shot publisher (safe and minimal) -----------
static std::string brokerUrl() {
    if (const char* env = std::getenv("BROKER_URL")) {
        return std::string(env);
    }
    return std::string("failover://(tcp://artemis:61616)");
}

static void publish_score_recorded(const std::string& tournamentId,
                                   const std::string& matchId) {
    // When DISABLE_SCORE_PUBLISH is set, skip broker calls (useful for tests)
    if (is_score_publish_disabled()) {
        std::cerr << "[MatchController] score publish disabled by env (DISABLE_SCORE_PUBLISH)"
                  << std::endl;
        return;
    }

    try {
        std::unique_ptr<cms::ConnectionFactory> factory(
            cms::ConnectionFactory::createCMSConnectionFactory(brokerUrl()));
        std::unique_ptr<cms::Connection> connection(factory->createConnection());
        connection->start();

        std::unique_ptr<cms::Session> session(
            connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));

        std::unique_ptr<cms::Destination> dest(
            session->createQueue("match.score-recorded"));

        std::unique_ptr<cms::MessageProducer> producer(
            session->createProducer(dest.get()));
        producer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);

        nlohmann::json j = {
            {"type", "match.score-recorded"},
            {"tournamentId", tournamentId},
            {"matchId", matchId}
        };

        std::unique_ptr<cms::TextMessage> msg(
            session->createTextMessage(j.dump()));
        producer->send(msg.get());

        std::cerr << "[MatchController] published match.score-recorded "
                  << matchId << " in " << tournamentId << std::endl;
    } catch (const cms::CMSException& e) {
        std::cerr << "[MatchController] ERROR publishing (CMS): "
                  << e.getMessage() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[MatchController] ERROR publishing: "
                  << e.what() << std::endl;
    }
}

// ------------------- Endpoints -------------------

// GET /tournaments/{tId}/matches?showMatches=played|pending
crow::response MatchController::ReadAll(const crow::request& request,
                                        const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, UUID_RE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    try {
        std::optional<std::string_view> filter;
        if (const char* p = request.url_params.get("showMatches")) {
            filter = std::string_view{p};
        }

        auto matches = matchDelegate->ReadAll(tournamentId, filter);

        nlohmann::json body = nlohmann::json::array();
        for (const auto& m : matches) {
            if (m) body.push_back(*m);
        }

        crow::response res(body.dump());
        res.code = crow::OK;
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    } catch (const std::runtime_error& e) {
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
// Body: { "score": { "home": <int>, "visitor": <int> } }
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

    // Publish event so the consumer can advance the tournament
    publish_score_recorded(tournamentId, matchId);

    return crow::response{crow::NO_CONTENT};
}

// POST /tournaments/{tId}/matches
// Body: { "round": "...", "home":{id,name}, "visitor":{id,name} }
crow::response MatchController::Create(const crow::request& request,
                                       const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, UUID_RE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }
    auto body = nlohmann::json::parse(request.body);

    try {
        auto result = matchDelegate->Create(tournamentId, body);
        if (!result) {
            const std::string err = result.error();
            if (err == "not_found") {
                return crow::response{crow::NOT_FOUND, "tournament not found"};
            }
            if (err.rfind("validation:", 0) == 0) {
                return crow::response{422, err.substr(std::string("validation:").size())};
            }
            return crow::response{crow::INTERNAL_SERVER_ERROR, "create match failed"};
        }

        nlohmann::json resp = { {"id", result.value()} };
        crow::response res(resp.dump());
        res.code = crow::CREATED;
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        res.add_header("Location",
            "/tournaments/" + tournamentId + "/matches/" + result.value());
        return res;
    } catch (...) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, "create match failed"};
    }
}

// Route bindings
REGISTER_ROUTE(MatchController, ReadAll,    "/tournaments/<string>/matches",            "GET"_method)
REGISTER_ROUTE(MatchController, ReadById,   "/tournaments/<string>/matches/<string>",  "GET"_method)
REGISTER_ROUTE(MatchController, PatchScore, "/tournaments/<string>/matches/<string>",  "PATCH"_method)
REGISTER_ROUTE(MatchController, Create,     "/tournaments/<string>/matches",           "POST"_method)
