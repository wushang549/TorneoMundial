// main.cpp
#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <iostream>

#include "configuration/ContainerSetup.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/ScoreUpdateListener.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::cout << "Starting tournament consumer...\n";
        auto container = config::containerSetup();
        std::cout << "Container initialized\n";

        auto teamAddListener  = container->resolve<GroupAddTeamListener>();
        auto matchDelegate    = container->resolve<MatchDelegate>();
        auto connectionMgr    = container->resolve<ConnectionManager>();
        auto scoreListener    = std::make_shared<ScoreUpdateListener>(connectionMgr, matchDelegate);

        std::thread t1([l = teamAddListener]() { l->Start("tournament.team-add"); });
        std::thread t2([l = scoreListener  ]() { l->Start("match.score-recorded"); }); // <-- aquí

        std::cout << "Listener threads started\n";
        t1.join();
        t2.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}
