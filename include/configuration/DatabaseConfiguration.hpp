#ifndef DATA_BASE_CONFIGURATION_HPP
#define DATA_BASE_CONFIGURATION_HPP

#include <string>

namespace config {
    struct DatabaseConfiguration{
        std::string connectionString;
    };

    inline void from_json(const nlohmann::json& json, DatabaseConfiguration& databaseConfiguration) {
        json.at("connectionString").get_to(databaseConfiguration.connectionString);
    }
}
#endif
