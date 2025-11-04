//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>

// App config
#include "RunConfiguration.hpp"

// DB
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"

// Repositories
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"

// Messaging
#include "cms/ConnectionManager.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/QueueResolver.hpp"

// Delegates
#include "delegate/ITeamDelegate.hpp"
#include "delegate/TeamDelegate.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "delegate/GroupDelegate.hpp"

// Controllers
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        // ---- Load configuration.json
        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        // ---- RunConfiguration
        auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
        builder.registerInstance(appConfig);

        // ---- Postgres connection provider
        auto pgProvider = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>()
        );
        builder.registerInstance(pgProvider).as<IDbConnectionProvider>();

        // ---- Messaging (ActiveMQ)
        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&, const std::shared_ptr<ConnectionManager>& instance) {
                instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
            })
            .singleInstance();

        // Producer “tournamentAddTeamQueue” (ajusta si usas más)
        builder.registerType<QueueMessageProducer>()
               .named("tournamentAddTeamQueue");

        // Resolver de colas
        builder.registerType<QueueResolver>()
               .as<IResolver<IQueueMessageProducer>>()
               .named("queueResolver")
               .singleInstance();

        // ---- Repositories
        builder.registerType<TeamRepository>()
               .as<IRepository<domain::Team, std::string_view>>()
               .singleInstance();

        builder.registerType<GroupRepository>()
               .as<IGroupRepository>()
               .singleInstance();

        builder.registerType<TournamentRepository>()
               .as<IRepository<domain::Tournament, std::string>>()   // NOTE: std::string as Id
               .singleInstance();

        // ---- Delegates
        builder.registerType<TeamDelegate>()
               .as<ITeamDelegate>()
               .singleInstance();

        builder.registerType<GroupDelegate>()
               .as<IGroupDelegate>()
               .singleInstance();

        builder.registerType<TournamentDelegate>()
               .as<ITournamentDelegate>()
               .singleInstance();

        // ---- Controllers
        builder.registerType<TeamController>()
               .singleInstance();

        builder.registerType<GroupController>()
               .singleInstance();

        builder.registerType<TournamentController>()
               .singleInstance();

        return builder.build();
    }

    inline std::shared_ptr<TeamController>
 makeTeamController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<TeamController>();  // <-- NO pidas shared_ptr<T>, solo T
    }

    inline std::shared_ptr<TournamentController>
    makeTournamentController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<TournamentController>();  // idem
    }

    inline std::shared_ptr<GroupController>
    makeGroupController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<GroupController>();  // idem
    }


} // namespace config

#endif // RESTAPI_CONTAINER_SETUP_HPP
