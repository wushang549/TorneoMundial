#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <iostream>

#include "configuration/ContainerSetup.hpp"
#include "cms/GroupAddTeamListener.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::cout << "Starting tournament consumer...\n";
        auto container = config::containerSetup();
        std::cout << "Container initialized\n";

        auto teamAddListener = container->resolve<GroupAddTeamListener>();

        std::thread t1([listener = std::move(teamAddListener)]() {
            listener->Start("tournament.team-add");
        });

        std::cout << "Listener thread started\n";
        t1.join();
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}
