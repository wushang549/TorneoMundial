//containersetup.hpp
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

// -------- Matches (NEW) --------
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "delegate/MatchDelegate.hpp"
#include "controller/MatchController.hpp"
// --------------------------------

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        // Load configuration.json
        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        // RunConfiguration
        auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
        builder.registerInstance(appConfig);

        // Postgres connection provider
        auto pgProvider = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>()
        );
        builder.registerInstance(pgProvider).as<IDbConnectionProvider>();

        // Messaging (ActiveMQ)
        builder.registerType<ConnectionManager>()
            .onActivated([configuration](Hypodermic::ComponentContext&, const std::shared_ptr<ConnectionManager>& instance) {
                const auto broker = configuration["activemq"]["broker-url"].get<std::string>();
                const auto user   = configuration["activemq"].value("username", std::string{});
                const auto pass   = configuration["activemq"].value("password", std::string{});
                const auto cid    = configuration["activemq"].value("clientId", std::string{"tournament-services"});
                instance->initialize(broker, user, pass, cid); // overload with creds+clientId
            })
            .singleInstance();


        // Producer as interface
        builder.registerType<QueueMessageProducer>()
               .as<IQueueMessageProducer>()
               .singleInstance();


        // Queue resolver
        builder.registerType<QueueResolver>()
               .as<IResolver<IQueueMessageProducer>>()
               .named("queueResolver")
               .singleInstance();

        // ----- Repositories -----
        builder.registerType<TeamRepository>()
               .as<IRepository<domain::Team, std::string_view>>()
               .singleInstance();

        builder.registerType<GroupRepository>()
               .as<IGroupRepository>()
               .singleInstance();

        builder.registerType<TournamentRepository>()
               .as<IRepository<domain::Tournament, std::string>>()
               .singleInstance();

        // Matches repo (NEW)
        builder.registerType<MatchRepository>()
               .as<IMatchRepository>()
               .singleInstance();

        // ----- Delegates -----
        builder.registerType<TeamDelegate>()
               .as<ITeamDelegate>()
               .singleInstance();

        builder.registerType<GroupDelegate>()
               .as<IGroupDelegate>()
               .singleInstance();

        builder.registerType<TournamentDelegate>()
               .as<ITournamentDelegate>()
               .singleInstance();

        // Matches delegate (NEW)
        builder.registerType<MatchDelegate>()
               .as<IMatchDelegate>();

        // ----- Controllers -----
        builder.registerType<TeamController>()
               .singleInstance();

        builder.registerType<GroupController>()
               .singleInstance();

        builder.registerType<TournamentController>()
               .singleInstance();

        // Matches controller (NEW)
        builder.registerType<MatchController>()
               .singleInstance();

        return builder.build();
    }

    inline std::shared_ptr<TeamController>
    makeTeamController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<TeamController>();
    }

    inline std::shared_ptr<TournamentController>
    makeTournamentController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<TournamentController>();
    }

    inline std::shared_ptr<GroupController>
    makeGroupController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<GroupController>();
    }

    // Optional factory for MatchController
    inline std::shared_ptr<MatchController>
    makeMatchController(const std::shared_ptr<Hypodermic::Container>& c) {
        return c->resolve<MatchController>();
    }


} // namespace config

#endif // RESTAPI_CONTAINER_SETUP_HPP
