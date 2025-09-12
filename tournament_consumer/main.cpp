//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>

#include "configuration/ContainerSetup.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    const auto container = config::containerSetup();
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}