//MatchController.hpp
#pragma once
#include <memory>
#include <string>
#include "crow.h"
#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"
// Forward declaration: the controller depends on the delegate interface,
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
    // POST /tournaments/{tId}/matches
    // Body: { "round": "...", "home":{id,name}, "visitor":{id,name} }
    crow::response Create(const crow::request& request,
                          const std::string& tournamentId) const;   // <-- NEW
};
