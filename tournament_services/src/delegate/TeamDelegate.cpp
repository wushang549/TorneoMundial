// delegate/TeamDelegate.cpp
#include "delegate/TeamDelegate.hpp"
#include <utility>
#include <string_view>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
    : teamRepository(std::move(repository)) {}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}
bool TeamDelegate::UpdateTeam(std::string_view id, const domain::Team& incoming) {
    if (!teamRepository->ReadById(std::string(id))) return false;  // no existe → 404 en controller
    domain::Team toUpdate = incoming;
    toUpdate.Id = std::string(id);   // 👈 forzamos el id del path
    teamRepository->Update(toUpdate);
    return true;
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    // ✅ usar string_view directo (coincide con la interfaz y con el mock)
    return teamRepository->ReadById(id);
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team) {
    return teamRepository->Create(team);
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



