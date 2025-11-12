//GroupDelegate.hpp
#pragma once
#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "delegate/IGroupDelegate.hpp"

// OJO: usa el prefijo real de tus headers:

#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "../cms/QueueMessageProducer.hpp"

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IGroupRepository>     groupRepository;
    std::shared_ptr<TeamRepository>       teamRepository;
    std::shared_ptr<QueueMessageProducer> messageProducer; // 🔹 falta este atributo


public:
    GroupDelegate(const std::shared_ptr<TournamentRepository>& tournamentRepository,
               const std::shared_ptr<IGroupRepository>& groupRepository,
               const std::shared_ptr<TeamRepository>& teamRepository,
               const std::shared_ptr<QueueMessageProducer>& messageProducer);


    // IGroupDelegate
    std::expected<std::string, std::string>
    CreateGroup(std::string_view tournamentId, const domain::Group& group) override;

    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
    GetGroups(std::string_view tournamentId) override;

    std::expected<std::shared_ptr<domain::Group>, std::string>
    GetGroup(std::string_view tournamentId, std::string_view groupId) override;

    std::expected<void, std::string>
    UpdateGroup(std::string_view tournamentId, const domain::Group& group) override;

    std::expected<void, std::string>
    RemoveGroup(std::string_view tournamentId, std::string_view groupId) override;

    std::expected<void, std::string>
    UpdateTeams(std::string_view tournamentId, std::string_view groupId,
                const std::vector<domain::Team>& teams) override;

    // Implementación que te faltaba
    std::expected<void, std::string>
    AddTeamToGroup(std::string_view tournamentId,
                   std::string_view groupId,
                   std::string_view teamId) override;
};

#endif /* SERVICE_GROUP_DELEGATE_HPP */
