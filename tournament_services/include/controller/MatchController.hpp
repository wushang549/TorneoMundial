#pragma once
#include <memory>
#include <string>
#include "crow.h"

// Forward declaration: the controller depends on the delegate interface,
// but we don't need the full definition in this header.
class IMatchDelegate;

/**
 * HTTP controller for Match endpoints.
 * - Parses/validates HTTP inputs
 * - Maps delegate results to HTTP responses
 * - Serializes responses to JSON
 *
 * Business logic remains in the delegate layer.
 */
class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;

public:
    explicit MatchController(std::shared_ptr<IMatchDelegate> d)
        : matchDelegate(std::move(d)) {}

    // GET /tournaments/{tId}/matches?showMatches=played|pending
    crow::response ReadAll(const crow::request& request,
                           const std::string& tournamentId) const;

    // GET /tournaments/{tId}/matches/{mId}
    crow::response ReadById(const std::string& tournamentId,
                            const std::string& matchId) const;

    // PATCH /tournaments/{tId}/matches/{mId}
    // Body: { "score": { "home": <int>, "visitor": <int> } }
    crow::response PatchScore(const crow::request& request,
                              const std::string& tournamentId,
                              const std::string& matchId) const;
};
