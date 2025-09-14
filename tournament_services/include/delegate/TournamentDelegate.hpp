//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>

#include "cms/IQueueMessageProducer.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "configuration/IResolver.hpp"
#include "persistence/repository/TournamentRepository.hpp"

class TournamentDelegate final : public ITournamentDelegate {
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IResolver<IQueueMessageProducer>> queueResolver;
public:
    explicit TournamentDelegate(const std::shared_ptr<TournamentRepository>& repository, const std::shared_ptr<IResolver<IQueueMessageProducer>>& queueResolver);
    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP