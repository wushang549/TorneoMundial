// MatchController.hpp
#pragma once
#include <memory>
#include <string>
#include "crow.h"
#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "cms/ConnectionManager.hpp"   // <-- add

class MatchController {
    std::shared_ptr<IMatchDelegate>   matchDelegate;
    std::shared_ptr<ConnectionManager> connectionManager; // <-- add

public:
    explicit MatchController(std::shared_ptr<IMatchDelegate> d)
        : matchDelegate(std::move(d)) {}

    // Minimal extra ctor to inject the ConnectionManager
    MatchController(std::shared_ptr<IMatchDelegate> d,
                    std::shared_ptr<ConnectionManager> cm)            // <-- add
        : matchDelegate(std::move(d)),
          connectionManager(std::move(cm)) {}

    crow::response ReadAll(const crow::request& request,
                           const std::string& tournamentId) const;

    crow::response ReadById(const std::string& tournamentId,
                            const std::string& matchId) const;

    crow::response PatchScore(const crow::request& request,
                              const std::string& tournamentId,
                              const std::string& matchId) const;

    crow::response Create(const crow::request& request,
                          const std::string& tournamentId) const;
};
