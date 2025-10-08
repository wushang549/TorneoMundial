// delegate/ITeamDelegate.hpp
#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include "domain/Team.hpp"

/// Interfaz de la capa Delegate para CRUD de Team.
class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;

    // Read
    virtual std::shared_ptr<domain::Team> GetTeam(std::string_view id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> GetAllTeams() = 0;

    virtual std::string_view SaveTeam(const domain::Team& team) = 0;

    // Update / Delete
    virtual bool UpdateTeam(std::string_view id, const domain::Team& team) = 0;
    virtual bool DeleteTeam(std::string_view id) = 0;
};

#endif /* ITEAM_DELEGATE_HPP */