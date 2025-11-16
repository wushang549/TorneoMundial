//ScoreUpdateListener
#ifndef LISTENER_SCOREUPDATE_LISTENER_HPP
#define LISTENER_SCOREUPDATE_LISTENER_HPP

#include <iostream>
#include <nlohmann/json.hpp>
#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/ScoreUpdateEvent.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;


public:
    void processMessage(const std::string& message) override;
    ScoreUpdateListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                        const std::shared_ptr<MatchDelegate>& matchDelegate);
    ~ScoreUpdateListener() override;
};

inline ScoreUpdateListener::ScoreUpdateListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate>& matchDelegate)
    : QueueMessageListener(connectionManager),
      matchDelegate(matchDelegate) {
    std::cout << "[ScoreUpdateListener] created with MatchDelegate" << std::endl;
}

inline ScoreUpdateListener::~ScoreUpdateListener() {
    Stop();
}

inline void ScoreUpdateListener::processMessage(const std::string& message) {
    std::cout << "[ScoreUpdateListener] Received message: " << message << std::endl;
    try {
        auto json = nlohmann::json::parse(message);
        if (!json.contains("tournamentId") || !json.contains("matchId")) {
            std::cout << "[ScoreUpdateListener] Missing fields\n";
            return;
        }
        const std::string tournamentId = json.at("tournamentId").get<std::string>();
        const std::string matchId      = json.at("matchId").get<std::string>();

        if (!matchDelegate) {
            std::cout << "[ScoreUpdateListener] ERROR: matchDelegate is null!\n";
            return;
        }
        matchDelegate->ProcessScoreUpdate(ScoreUpdateEvent{tournamentId, matchId});
    } catch (const std::exception& e) {
        std::cout << "[ScoreUpdateListener] ERROR processing message: " << e.what() << std::endl;
    }
}


#endif // LISTENER_SCOREUPDATE_LISTENER_HPP
