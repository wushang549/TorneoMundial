#ifndef LISTENER_MATCHCREATION_LISTENER_HPP
#define LISTENER_MATCHCREATION_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include <iostream>
#include <string>

class MatchCreationListener : public QueueMessageListener {
protected:
    // Now protected so a test subclass can expose it
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
    std::cout << "Match created: " << message << std::endl;
}

#endif
