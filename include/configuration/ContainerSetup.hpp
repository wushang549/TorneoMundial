//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP
#include <Hypodermic/ContainerBuilder.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

#include "DatabaseConfiguration.hpp"
#include "persistence/reposiotry/IRepository.hpp"
#include "persistence/reposiotry/TeamRepository.hpp"
#include "RunConfiguration.hpp"
#include "delegate/TeamDelegate.hpp"
#include "controller/TeamController.hpp"



inline std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;

    std::ifstream file("configuration.json");
    if(file.is_open()) {
        nlohmann::json jsonData;
        file >> jsonData;
        std::shared_ptr<config::RunConfiguration> appConfig = std::make_shared<config::RunConfiguration>(jsonData["runConfig"]);
        builder.registerInstance(appConfig);

        std::shared_ptr<config::DatabaseConfiguration> dbConfig = std::make_shared<config::DatabaseConfiguration>(jsonData["databaseConfig"]);
        builder.registerInstance(dbConfig);
    }

    builder.registerType<TeamRepositroy>.as<IRepository>().singleInstance();
    builder.registerType<TeamDelegate>().singleInstance();
    builder.registerType<TeamController>().singleInstance();

    return builder.build();
}
#endif //RESTAPI_CONTAINER_SETUP_HPP