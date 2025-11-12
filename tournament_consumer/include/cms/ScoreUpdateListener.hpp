#ifndef LISTENER_SCOREUPDATE_LISTENER_HPP
#define LISTENER_SCOREUPDATE_LISTENER_HPP

#include <iostream>
#include <nlohmann/json.hpp>
#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/ScoreUpdateEvent.hpp"

class ScoreUpdateListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;

    void processMessage(const std::string& message) override;

public:
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

        std::string tournamentId = json["tournamentId"];
        std::string matchId      = json["matchId"];

        std::cout << "[ScoreUpdateListener] Updating score in match "
                  << matchId << " in tournament " << tournamentId << std::endl;

        if (!matchDelegate) {
            std::cout << "[ScoreUpdateListener] ERROR: matchDelegate is null!" << std::endl;
            return;
        }

        ScoreUpdateEvent scoreUpdateEvent{tournamentId, matchId};
        matchDelegate->ProcessScoreUpdate(scoreUpdateEvent);

    } catch (const std::exception& e) {
        std::cout << "[ScoreUpdateListener] ERROR processing message: "
                  << e.what() << std::endl;
    }
}

#endif // LISTENER_SCOREUPDATE_LISTENER_HPP
