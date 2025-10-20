#include "delegate/TournamentDelegate.hpp"

std::expected<std::string, std::string>
TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    try {
        return tournamentRepository->Create(*tournament);
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to create tournament: ") + ex.what());
    }
}

std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>
TournamentDelegate::ReadAll() {
    try {
        return tournamentRepository->ReadAll();
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to read tournaments: ") + ex.what());
    }
}

std::expected<std::shared_ptr<domain::Tournament>, std::string>
TournamentDelegate::ReadById(const std::string& id) {
    try {
        return tournamentRepository->ReadById(id);
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to read tournament: ") + ex.what());
    }
}

std::expected<bool, std::string>
TournamentDelegate::UpdateTournament(const std::string& id, const domain::Tournament& t) {
    try {
        if (!tournamentRepository->ReadById(id)) {
            return false; // not found
        }

        domain::Tournament copy = t;
        copy.Id() = id;
        tournamentRepository->Update(copy);
        return true;
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to update tournament: ") + ex.what());
    }
}

std::expected<bool, std::string>
TournamentDelegate::DeleteTournament(const std::string& id) {
    try {
        if (!tournamentRepository->ReadById(id)) {
            return false; // not found
        }
        tournamentRepository->Delete(id);
        return true;
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to delete tournament: ") + ex.what());
    }
}
