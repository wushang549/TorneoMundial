//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTCONTROLLER_HPP
#define TOURNAMENTS_TOURNAMENTCONTROLLER_HPP

#include <memory>
#include <crow.h>

#include "delegate/ITournamentDelegate.hpp"


class TournamentController {
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;
public:
    explicit TournamentController(std::shared_ptr<ITournamentDelegate> tournament);
    crow::response CreateTournament(const crow::request &request) const;
};


#endif //TOURNAMENTS_TOURNAMENTCONTROLLER_HPP