//
// Created by tomas on 8/24/25.
//

#ifndef TOURNAMENTS_UTILITIES_HPP
#define TOURNAMENTS_UTILITIES_HPP
#include <nlohmann/json_fwd.hpp>

#include "domain/Team.hpp"


namespace utilities {

    std::shared_ptr<nlohmann::json> toTeamJson(const domain::Team & team) {
        auto json = std::shared_ptr<nlohmann::json>();
        (*json)["id"] = team.Id;

        return json;
    }
}
#endif //TOURNAMENTS_UTILITIES_HPP