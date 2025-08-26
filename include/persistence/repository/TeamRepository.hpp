//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>
#include <memory>

#include "IRepository.hpp"
#include "domain/Team.hpp"

// template <typename T, typename Id>
class TeamRepository : public IRepository<domain::Team, std::string> {
public:

    std::vector<std::shared_ptr<domain::Team>> ReadAll() {
        std::vector<std::shared_ptr<domain::Team>> teams;
        teams.push_back(std::make_shared<domain::Team>(domain::Team{"ID", "NAME"}));
        
        teams.push_back(std::make_shared<domain::Team>(domain::Team{"USA", "United States"}));
        teams.push_back(std::make_shared<domain::Team>(domain::Team{"CA", "Canada"}));
        teams.push_back(std::make_shared<domain::Team>(domain::Team{"MX", "Mexico"}));

        return teams;
    }

    std::shared_ptr<domain::Team> ReadById(std::string id) {
        return std::make_shared<domain::Team>();
    }

    std::string Save(const domain::Team &entity) {
        return "newID";
    }

    void Delete(std::string id) {
        
    }
};


#endif //RESTAPI_TEAMREPOSITORY_HPP