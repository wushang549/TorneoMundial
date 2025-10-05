#include "delegate/TournamentDelegate.hpp"

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    // business rules (opcional)...
    return tournamentRepository->Create(*tournament);
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string& id) {
    return tournamentRepository->ReadById(id);
}

bool TournamentDelegate::UpdateTournament(const std::string& id, const domain::Tournament& t) {
    domain::Tournament copy = t;
    copy.Id() = id;
    try {
        tournamentRepository->Update(copy);
        return true;
    } catch (...) {
        return false;
    }
}

bool TournamentDelegate::DeleteTournament(const std::string& id) {
    try {
        tournamentRepository->Delete(id);
        return true;
    } catch (...) {
        return false;
    }
}
