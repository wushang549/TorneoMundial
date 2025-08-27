#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string>
#include <memory>

#include "domain/Team.hpp"

class ITeamDelegate {
    public:
    virtual ~ITeamDelegate() = default;
    virtual std::shared_ptr<domain::Team> getTeam(std::string_view id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> getAllTeams() = 0;
};

#endif /* ITEAM_DELEGATE_HPP */
