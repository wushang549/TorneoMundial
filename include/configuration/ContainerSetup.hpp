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
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "RunConfiguration.hpp"
#include "delegate/TeamDelegate.hpp"
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"

namespace config {
    inline std::shared_ptr<Hypodermic::Container> containerSetup(){
                Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        if(file.is_open()) {
            nlohmann::json configuration;
            file >> configuration;
            std::shared_ptr<config::RunConfiguration> appConfig = std::make_shared<config::RunConfiguration>(configuration["runConfig"]);
            builder.registerInstance(appConfig);
          
            std::shared_ptr<PostgresConnectionProvider> postgressConnection = std::make_shared<PostgresConnectionProvider>(configuration["databaseConfig"]["connectionString"].get<std::string>(), configuration["databaseConfig"]["poolSize"].get<size_t>());
            builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();
        }

        builder.registerType<TeamRepository>().as<IRepository<domain::Team, std::string_view>>();

        builder.registerType<TeamDelegate>().as<ITeamDelegate>().singleInstance();
        builder.registerType<TeamController>().singleInstance();

        builder.registerType<TournamentDelegate>().as<ITournamentDelegate>().singleInstance();
        builder.registerType<TournamentController>().singleInstance();

        return builder.build();
    }
}
#endif //RESTAPI_CONTAINER_SETUP_HPP