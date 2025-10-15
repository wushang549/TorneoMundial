#include "delegate/GroupDelegate.hpp"
#include <algorithm>
#include <stdexcept>

// ---------------- ctor ----------------
GroupDelegate::GroupDelegate(const std::shared_ptr<TournamentRepository>& tournamentRepository,
                             const std::shared_ptr<IGroupRepository>& groupRepository,
                             const std::shared_ptr<TeamRepository>& teamRepository)
    : tournamentRepository(tournamentRepository),
      groupRepository(groupRepository),
      teamRepository(teamRepository) {}

// ---------------- CreateGroup ----------------
std::expected<std::string, std::string>
GroupDelegate::CreateGroup(std::string_view tournamentId, const domain::Group& group) {
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    domain::Group g = group;
    g.TournamentId() = tournament->Id();

    // Validate teams list if provided
    if (!g.Teams().empty()) {
        for (auto& t : g.Teams()) {
            // Según tu modelo previo, t puede tener .Id campo
            auto team = teamRepository->ReadById(t.Id);
            if (!team) {
                return std::unexpected("Team doesn't exist");
            }
        }
    }

    try {
        auto id = groupRepository->Create(g);
        return id;
    } catch (const std::exception&) {
        return std::unexpected("Failed to create group");
    }
}

// ---------------- GetGroups ----------------
std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(std::string_view tournamentId) {
    try {
        return groupRepository->FindByTournamentId(tournamentId);
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

// ---------------- GetGroup ----------------
std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(std::string_view tournamentId, std::string_view groupId) {
    try {
        return groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

// ---------------- UpdateGroup / RemoveGroup (stubs) ----------------
std::expected<void, std::string>
GroupDelegate::UpdateGroup(std::string_view, const domain::Group&) {
    return std::unexpected("Not implemented");
}

std::expected<void, std::string>
GroupDelegate::RemoveGroup(std::string_view, std::string_view) {
    return std::unexpected("Not implemented");
}

// ---------------- UpdateTeams (batch) ----------------
std::expected<void, std::string>
GroupDelegate::UpdateTeams(std::string_view tournamentId,
                           std::string_view groupId,
                           const std::vector<domain::Team>& teams) {
    // Tournament (for capacity rule)
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (!tournament) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Group
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group doesn't exist");
    }

    // Capacity
    const int maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    const std::size_t current  = group->Teams().size();
    const std::size_t incoming = teams.size();
    if (static_cast<int>(current + incoming) > maxPerGroup) {
        return std::unexpected("Group at max capacity");
    }

    // Prevent duplicates across the tournament
    for (const auto& team : teams) {
        auto existing = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id);
        if (existing) {
            return std::unexpected("Team " + team.Id +
                                   " already exists in tournament " + std::string(tournamentId));
        }
    }

    // Persist teams
    try {
        for (const auto& team : teams) {
            auto persistedTeam = teamRepository->ReadById(team.Id);
            if (!persistedTeam) {
                return std::unexpected("Team " + team.Id + " doesn't exist");
            }
            groupRepository->UpdateGroupAddTeam(std::string(groupId), persistedTeam);
        }
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to update teams: ") + ex.what());
    }

    return {};
}

// ---------------- AddTeamToGroup (single) ----------------
std::expected<void, std::string>
GroupDelegate::AddTeamToGroup(std::string_view tournamentId,
                              std::string_view groupId,
                              std::string_view teamId) {
    // 1) Validate existence
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

    // 2) Idempotency: already present anywhere in this tournament?
    auto existing = groupRepository->FindByTournamentIdAndTeamId(tournamentId, teamId);
    if (existing) {
        return {}; // already assigned somewhere in the tournament
    }

    // 3) Capacity check for this group based on tournament format
    const int maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (static_cast<int>(group->Teams().size()) >= maxPerGroup) {
        return std::unexpected("Group is full");
    }

    // 4) Persist
    try {
        groupRepository->UpdateGroupAddTeam(std::string(groupId), team);
    } catch (const std::exception& ex) {
        return std::unexpected(std::string("Failed to add team: ") + ex.what());
    }

    return {};
}
