#ifndef LISTENER_MATCHCREATION_LISTENER_HPP
#define LISTENER_MATCHCREATION_LISTENER_HPP

#include "QueueMessageListener.hpp"

class MatchCreationListener : public QueueMessageListener {
    void processMessage(const std::string& message) override;

public:
    MatchCreationListener(const std::shared_ptr<ConnectionManager>& connectionManager);
    ~MatchCreationListener() override;
};

inline MatchCreationListener::MatchCreationListener(
    const std::shared_ptr<ConnectionManager>& connectionManager)
    : QueueMessageListener(connectionManager) {}

inline MatchCreationListener::~MatchCreationListener() {
    Stop();
}

inline void MatchCreationListener::processMessage(const std::string& message) {
    std::cout<<("Match created: {}", message)<< std::endl;
    // Este listener solo registra que se creó un match
    // No necesita hacer nada más por ahora
}

#endif