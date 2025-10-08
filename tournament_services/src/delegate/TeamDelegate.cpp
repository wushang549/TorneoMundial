// delegate/TeamDelegate.cpp
#include "delegate/TeamDelegate.hpp"
#include <utility>
#include <string_view>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
    : teamRepository(std::move(repository)) {}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    // ✅ usar string_view directo (coincide con la interfaz y con el mock)
    return teamRepository->ReadById(id);
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team) {
    return teamRepository->Create(team);
}

bool TeamDelegate::UpdateTeam(std::string_view id, const domain::Team& team) {
    // ✅ pre-check con string_view
    if (teamRepository->ReadById(id) == nullptr) {
        return false;
    }
    // IRepository::Update devuelve Id (string_view); no lo usamos aquí
    (void)teamRepository->Update(team);
    return true;
}

bool TeamDelegate::DeleteTeam(std::string_view id) {
    // ✅ pre-check con string_view
    if (teamRepository->ReadById(id) == nullptr) {
        return false; // not found
    }
    // ✅ delete con string_view
    teamRepository->Delete(id);
    // ✅ post-check con string_view
    return teamRepository->ReadById(id) == nullptr;
}



