//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include "delegate/ITournamentDelegate.hpp"

class TournamentDelegate : public ITournamentDelegate{
public:
    std::string_view CreateTournament(const domain::Tournament& tournament) override;;
};

std::string_view TournamentDelegate::CreateTournament(const domain::Tournament& tournament) {

    return "new-id";
}
#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP