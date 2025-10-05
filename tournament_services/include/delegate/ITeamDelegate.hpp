#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string_view>
#include <memory>
#include <vector>
#include "domain/Team.hpp"

class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;

    virtual std::shared_ptr<domain::Team> GetTeam(std::string_view id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> GetAllTeams() = 0;
    virtual std::string_view SaveTeam(const domain::Team& team) = 0;           // Create

    // New for full CRUD:
    virtual bool UpdateTeam(std::string_view id, const domain::Team& team) = 0; // Update
    virtual bool DeleteTeam(std::string_view id) = 0;                           // Delete
};

#endif /* ITEAM_DELEGATE_HPP */
