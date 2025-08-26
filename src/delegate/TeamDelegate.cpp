//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string> > repository) : teamRepository(repository) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::getAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::getTeam(std::string_view id) {
    return nullptr;
}


