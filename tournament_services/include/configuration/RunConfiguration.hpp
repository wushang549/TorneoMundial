#ifndef TOURNAMENTS_APPLICATION_PROPERTIES_HPP
#define TOURNAMENTS_APPLICATION_PROPERTIES_HPP
#include <nlohmann/json.hpp>

namespace config{
    struct RunConfiguration{
        int port;
        int concurrency;
    };

    inline void from_json(const nlohmann::json& json, RunConfiguration& applicationProperties) {
        json.at("port").get_to(applicationProperties.port);
        json.at("concurrency").get_to(applicationProperties.concurrency);
    }
}
#endif
