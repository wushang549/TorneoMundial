//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"
#include "persistence/model/Team.hpp"


class TeamDelegate {
    std::shared_ptr<IRepository<persistence::Team, std::string>> teamRepository;
    public:
    TeamDelegate(std::shared_ptr<IRepository<persistence::Team, std::string>> repository);
};


#endif //RESTAPI_TESTDELEGATE_HPP