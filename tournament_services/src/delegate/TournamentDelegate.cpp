#include "delegate/TournamentDelegate.hpp"
#include <utility>

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
    std::shared_ptr<QueueMessageProducer> producer)
    : tournamentRepository(std::move(repository)), producer(std::move(producer)) {}

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    const std::string id = tournamentRepository->Create(*tp);
    if (producer) {
        producer->SendMessage(id, "tournament.created");
    }
    return id;
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

std::shared_ptr<domain::Tournament> TournamentDelegate::ReadById(const std::string& id) {
    return tournamentRepository->ReadById(id);
}

bool TournamentDelegate::UpdateTournament(const std::string& id, const domain::Tournament& tournament) {
    // Check existence first to emulate 404
    if (tournamentRepository->ReadById(id) == nullptr) {
        return false;
    }
    // IRepository::Update(const Type&) — no id param
    (void)tournamentRepository->Update(tournament);
    if (producer) {
        producer->SendMessage(id, "tournament.updated");
    }
    return true;
}

bool TournamentDelegate::DeleteTournament(const std::string& id) {
    // Check existence first
    if (tournamentRepository->ReadById(id) == nullptr) {
        return false;
    }
    // IRepository::Delete(const Id&) returns void
    tournamentRepository->Delete(id);
    if (producer) {
        producer->SendMessage(id, "tournament.deleted");
    }
    return true;
}
