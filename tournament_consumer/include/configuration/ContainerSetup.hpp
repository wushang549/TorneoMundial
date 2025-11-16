#ifndef TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
#define TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

// DB & repos
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"

// Delegate
#include "delegate/MatchDelegate.hpp"

// MQ
#include "cms/ConnectionManager.hpp"
#include "cms/GroupAddTeamListener.hpp"
#include "cms/ScoreUpdateListener.hpp"

namespace config {

inline std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;

    // Load configuration.json
    nlohmann::json configuration;
    {
        std::ifstream file("configuration.json");
        if (!file.is_open()) throw std::runtime_error("configuration.json not found");
        file >> configuration;
    }

    // Postgres provider (instance)
    auto pg = std::make_shared<PostgresConnectionProvider>(
        configuration["databaseConfig"]["connectionString"].get<std::string>(),
        configuration["databaseConfig"]["poolSize"].get<size_t>()
    );
    builder.registerInstance(pg).as<IDbConnectionProvider>();

    // ActiveMQ connection
    const std::string brokerUrl = configuration["activemq"]["broker-url"].get<std::string>();
    builder.registerType<ConnectionManager>()
        .onActivated([brokerUrl](Hypodermic::ComponentContext&,
                                 const std::shared_ptr<ConnectionManager>& cm) {
            cm->initialize(brokerUrl);
        })
        .singleInstance();

    // Repositories
    builder.registerType<GroupRepository>()
        .as<IGroupRepository>()
        .singleInstance();

    builder.registerType<MatchRepository>()
        .as<IMatchRepository>()
        .singleInstance();

    // Regístralo como INTERFAZ y también como CONCRETO (para ctors que piden concreto)
    builder.registerType<TournamentRepository>()
        .as<IRepository<domain::Tournament, std::string>>()
        .singleInstance();

    builder.registerType<TeamRepository>()
        .as<IRepository<domain::Team, std::string_view>>()
        .singleInstance();

    // Delegate y listeners (resolución por tipo concreto)
    builder.registerType<MatchDelegate>().singleInstance();
    builder.registerType<GroupAddTeamListener>().singleInstance();
    builder.registerType<ScoreUpdateListener>().singleInstance();

    return builder.build();
}

}

#endif // TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
