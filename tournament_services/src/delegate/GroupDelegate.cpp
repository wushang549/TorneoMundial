//GroupDelegate.cpp
#include "delegate/GroupDelegate.hpp"
#include "../include/cms/QueueMessageProducer.hpp"
#include <chrono>        // for timestamp
#include <nlohmann/json.hpp>
#include <algorithm>
// ---------------- ctor ----------------
GroupDelegate::GroupDelegate(
    const std::shared_ptr<TournamentRepository>& tRepo,
    const std::shared_ptr<IGroupRepository>& gRepo,
    const std::shared_ptr<TeamRepository>& teamRepo,
    const std::shared_ptr<IQueueMessageProducer>& producer)
    : tournamentRepository(tRepo),
      groupRepository(gRepo),
      teamRepository(teamRepo),
      messageProducer(producer) {}



// ---------------- CreateGroup ----------------
std::expected<std::string, std::string>
GroupDelegate::CreateGroup(std::string_view tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    domain::Group g = group;
    g.TournamentId() = tournament->Id();

    const int maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (static_cast<int>(g.Teams().size()) > maxPerGroup) {
        return std::unexpected("Group at max capacity");
    }

    if (!g.Teams().empty()) {
        for (auto& t : g.Teams()) {
            auto team = teamRepository->ReadById(t.Id);
            if (!team) {
                return std::unexpected("Team doesn't exist");
            }
            if (groupRepository->FindByTournamentIdAndTeamId(tournament->Id(), t.Id)) {
                return std::unexpected("Team " + t.Id + " already exists in tournament " + tournament->Id());
            }
        }
    }

    auto id = groupRepository->Create(g);
    if (id.empty()) {
        return std::unexpected("Failed to create group");
    }

    return id;
}

// ---------------- GetGroups ----------------
std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(std::string_view tournamentId) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    return groupRepository->FindByTournamentId(tournamentId);
}

// ---------------- GetGroup ----------------
std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(std::string_view tournamentId, std::string_view groupId) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group doesn't exist");
    }

    return group;
}

// ---------------- UpdateGroup (rename) ----------------
std::expected<void, std::string>
GroupDelegate::UpdateGroup(std::string_view tournamentId, const domain::Group& group) {
    if (group.Id().empty()) {
        return std::unexpected("Group id required");
    }
    if (group.Name().empty()) {
        return std::unexpected("Group name required");
    }

    // Validar torneo y grupo existen
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }
    auto current = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
    if (!current) {
        return std::unexpected("Group doesn't exist");
    }

    // Actualizar nombre
    domain::Group toUpdate = *current;
    toUpdate.Name() = group.Name();
    auto updatedId = groupRepository->Update(toUpdate);
    if (updatedId.empty()) {
        return std::unexpected("Failed to update group");
    }

    return {};
}

// ---------------- RemoveGroup ----------------
std::expected<void, std::string>
GroupDelegate::RemoveGroup(std::string_view tournamentId, std::string_view groupId) {
    // Validar torneo y grupo existen
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }
    auto current = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!current) {
        return std::unexpected("Group doesn't exist");
    }

    groupRepository->Delete(std::string(groupId));
    return {};
}

// ---------------- UpdateTeams (batch) ----------------
std::expected<void, std::string>
GroupDelegate::UpdateTeams(std::string_view tournamentId,
                           std::string_view groupId,
                           const std::vector<domain::Team>& teams) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group doesn't exist");
    }

    const int maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    const std::size_t current  = group->Teams().size();
    const std::size_t incoming = teams.size();
    if (static_cast<int>(current + incoming) > maxPerGroup) {
        return std::unexpected("Group at max capacity");
    }

    for (const auto& team : teams) {
        auto existing = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id);
        if (existing) {
            return std::unexpected("Team " + team.Id +
                                   " already exists in tournament " + std::string(tournamentId));
        }
    }

    for (const auto& team : teams) {
        auto persistedTeam = teamRepository->ReadById(team.Id);
        if (!persistedTeam) {
            return std::unexpected("Team " + team.Id + " doesn't exist");
        }
        groupRepository->UpdateGroupAddTeam(std::string(groupId), persistedTeam);
    }

    return {};
}

// ---------------- AddTeamToGroup (single) ----------------
std::expected<void, std::string>
GroupDelegate::AddTeamToGroup(std::string_view tournamentId,
                              std::string_view groupId,
                              std::string_view teamId) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group doesn't exist");
    }

    auto team = teamRepository->ReadById(std::string(teamId));
    if (!team) {
        return std::unexpected("Team doesn't exist");
    }

    auto existing = groupRepository->FindByTournamentIdAndTeamId(tournamentId, teamId);
    if (existing) {
        return std::unexpected("Team " + std::string(teamId) +
                               " already exists in tournament " + std::string(tournamentId));
    }

    const int maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (static_cast<int>(group->Teams().size()) >= maxPerGroup) {
        return std::unexpected("Group is full");
    }

    groupRepository->UpdateGroupAddTeam(std::string(groupId), team);

    // --- Publish domain event ---
    if (messageProducer) {
        nlohmann::json evt;
        evt["type"]         = "tournament.team.added";
        evt["tournamentId"] = std::string(tournamentId);
        evt["groupId"]      = std::string(groupId);
        evt["teamId"]       = std::string(teamId);
        evt["occurredAt"]   = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch()
                              ).count();

        // Queue name must match your consumer Start("tournament.team-add")
        messageProducer->SendMessage(evt.dump(), "tournament.team-add");
        std::cout << "[producer] published tournament.team-add: " << evt.dump() << std::endl;
    }

    return {};
}
