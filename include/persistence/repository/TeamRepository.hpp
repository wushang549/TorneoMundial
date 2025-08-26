//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>

#include "IRepository.hpp"
#include "domain/Team.hpp"

class TeamRepository : public IRepository<domain::Team, std::string> {
public:

    std::shared_ptr<domain::Team> ReadById( std::string id) override;

    std::string Save(const domain::Team &entity) override;

    void Delete(std::string id) override;

    std::vector<std::shared_ptr<domain::Team>> ReadAll() override;
};


#endif //RESTAPI_TEAMREPOSITORY_HPP