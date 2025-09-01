//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string_view>
#include <memory>

#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;
    virtual std::string_view CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP