//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP
#include <Hypodermic/ContainerBuilder.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <print>

#include "ApplicationProperties.hpp"
#include "controller/TeamController.hpp"


std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;
    builder.registerType<TeamController>().singleInstance();

    std::ifstream file("configuration.json");
    if(file.is_open()) {
        nlohmann::json jsonData;
        file >> jsonData;

        std::shared_ptr<ApplicationProperties> applicationProperties = std::make_shared<ApplicationProperties>(
              jsonData["application"].at("port").get<int>()
            , jsonData["application"].at("concurrency").get<int>()
        );
        builder.registerInstance(applicationProperties);
    }

    return builder.build();
}
#endif //RESTAPI_CONTAINER_SETUP_HPP