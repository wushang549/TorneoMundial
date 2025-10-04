//
// Created by tomas on 9/6/25.
//ewfewf
#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/ContainerSetup.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        std::thread tournamentCreatedThread([&] {
            auto listener = container->resolve<QueueMessageConsumer>();
            listener->Start("tournament.created");
        });

        tournamentCreatedThread.join();
        // while (true) {
        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}