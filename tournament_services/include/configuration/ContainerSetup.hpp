//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <activemq/core/ActiveMQConnectionFactory.h>
#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>


#include "configuration/DatabaseConfiguration.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "RunConfiguration.hpp"
#include "delegate/TeamDelegate.hpp"
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/MessageProducer.hpp"

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup(){
                Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        if(file.is_open()) {
            nlohmann::json configuration;
            file >> configuration;
            std::shared_ptr<RunConfiguration> appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
            builder.registerInstance(appConfig);
          
            std::shared_ptr<PostgresConnectionProvider> postgressConnection = std::make_shared<PostgresConnectionProvider>(configuration["databaseConfig"]["connectionString"].get<std::string>(), configuration["databaseConfig"]["poolSize"].get<size_t>());
            builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();

            
            builder.registerInstanceFactory(
                [&configuration](Hypodermic::ComponentContext& context) -> std::shared_ptr<activemq::core::ActiveMQConnectionFactory> {
                    return std::make_shared<activemq::core::ActiveMQConnectionFactory>(configuration["activemq"]["broker-url"].get<std::string>());
                }).singleInstance();

            // builder.registerInstanceFactory(
            //     [](Hypodermic::ComponentContext& context) ->  std::shared_ptr<cms::Connection> {
            //         auto connectionFactory = context.resolve<activemq::core::ActiveMQConnectionFactory>();
            //         auto connection = std::shared_ptr<cms::Connection>(connectionFactory->createConnection());
            //         connection->start();
            //         return connection;
            // });

            builder.registerInstanceFactory(
                [](Hypodermic::ComponentContext& context) ->  std::shared_ptr<QueueMessageProducer> {
                    auto connection =context.resolve<cms::Connection>();
                    const auto session = std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
                    const auto destination = std::shared_ptr<cms::Destination>(session->createQueue("tournament-add-team"));
                    
                    auto producer = std::shared_ptr<cms::MessageProducer>(session->createProducer(destination.get()));
                    producer->setDeliveryMode( cms::DeliveryMode::NON_PERSISTENT );
                    
                    return std::make_shared<QueueMessageProducer>(producer, session);
                });
        }

        //  builder.registerType<QueueMessageProducer>().singleInstance<>();
        
        builder.registerType<TeamRepository>().as<IRepository<domain::Team, std::string_view>>().singleInstance();

        builder.registerType<TeamDelegate>().as<ITeamDelegate>().singleInstance();
        builder.registerType<TeamController>().singleInstance();

        builder.registerType<TournamentRepository>().as<IRepository<domain::Tournament, std::string>>().singleInstance();
        builder.registerType<TournamentDelegate>().as<ITournamentDelegate>().singleInstance();
        builder.registerType<TournamentController>().singleInstance();

        return builder.build();
    }
}
#endif //RESTAPI_CONTAINER_SETUP_HPP