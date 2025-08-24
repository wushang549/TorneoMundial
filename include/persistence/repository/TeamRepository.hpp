//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>

#include "IRepository.hpp"
#include "persistence/model/Team.hpp"

class TeamRepository : public IRepository<persistence::Team, std::string> {
public:

    persistence::Team ReadById( std::string id) override;

    std::string Save(persistence::Team entity) override;

    void Delete(std::string id) override;

    std::vector<persistence::Team> ReadAll() override;
};


#endif //RESTAPI_TEAMREPOSITORY_HPP