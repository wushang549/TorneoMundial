//
// Created by tomas on 8/22/25.
//
#include "delegate/TeamDelegate.hpp"
#include <utility>
#include <string> // for std::string when converting string_view

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
    : teamRepository(std::move(repository)) {}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    // IRepository takes Id as std::string_view, but ReadById likely expects const char* or std::string
    // Use .data() only to probe if your IRepository has that overload; if not, wrap to std::string.
    return teamRepository->ReadById(id.data());
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team) {
    return teamRepository->Create(team);
}

// TeamDelegate.cpp
bool TeamDelegate::UpdateTeam(std::string_view id, const domain::Team& team) {
    if (teamRepository->ReadById(std::string{id}) == nullptr) {
        return false;
    }
    (void)teamRepository->Update(team); // IRepository::Update(const Type&)
    return true;
}

bool TeamDelegate::DeleteTeam(std::string_view id) {
    const std::string key{id}; // ensure exact type match with repository
    // pre-check to preserve 404 semantics
    if (teamRepository->ReadById(key) == nullptr) {
        return false; // not found
    }
    // perform delete
    teamRepository->Delete(key);
    // post-check: make sure it is really gone
    return teamRepository->ReadById(key) == nullptr;
}



