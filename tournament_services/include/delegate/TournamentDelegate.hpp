//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>

#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/IMessageProducer.hpp"

class TournamentDelegate : public ITournamentDelegate{
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<IMessageProducer> producer;
public:
    explicit TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository, std::shared_ptr<IMessageProducer> producer);

    std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP