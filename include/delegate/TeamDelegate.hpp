//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"

class TeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string>> teamRepository;
    public:
    TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string>> repository);
    std::shared_ptr<domain::Team> getTeam(std::string_view id);
    std::vector<std::shared_ptr<domain::Team>> getAllTeams();
};


#endif //RESTAPI_TESTDELEGATE_HPP