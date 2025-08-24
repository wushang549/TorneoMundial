//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP
#include <Hypodermic/ContainerBuilder.h>

#include "controller/TestController.hpp"

std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;
    builder.registerType<TestController>().singleInstance();
    return builder.build();
}
#endif //RESTAPI_CONTAINER_SETUP_HPP