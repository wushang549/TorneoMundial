#ifndef LISTENER_GROUPADDTEAM_LISTENER_HPP
#define LISTENER_GROUPADDTEAM_LISTENER_HPP

#include <iostream>
#include <nlohmann/json.hpp>
#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"
#include "event/TeamAddEvent.hpp"

class GroupAddTeamListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;

protected:
    void processMessage(const std::string& message) override;

public:
    GroupAddTeamListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                         const std::shared_ptr<MatchDelegate>& matchDelegate);
    ~GroupAddTeamListener() override;
};

inline GroupAddTeamListener::GroupAddTeamListener(
    const std::shared_ptr<ConnectionManager>& connectionManager,
    const std::shared_ptr<MatchDelegate>& matchDelegate)
    : QueueMessageListener(connectionManager),
      matchDelegate(matchDelegate) {
    std::cout << "[GroupAddTeamListener] created with MatchDelegate" << std::endl;
}

inline GroupAddTeamListener::~GroupAddTeamListener() {
    Stop();
}

inline void GroupAddTeamListener::processMessage(const std::string& message) {
    std::cout << "[GroupAddTeamListener] Received: " << message << std::endl;
    try {
        auto json = nlohmann::json::parse(message);
        TeamAddEvent evt{
            json.at("tournamentId").get<std::string>(),
            json.at("groupId").get<std::string>(),
            json.at("teamId").get<std::string>()
        };

        if (!matchDelegate) {
            std::cout << "[GroupAddTeamListener] ERROR: matchDelegate is null!" << std::endl;
            return;
        }

        matchDelegate->ProcessTeamAddition(evt);

    } catch (const std::exception& e) {
        std::cout << "[GroupAddTeamListener] ERROR: " << e.what() << std::endl;
    }
}

#endif // LISTENER_GROUPADDTEAM_LISTENER_HPP
