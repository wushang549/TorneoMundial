//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <format>
#include "delegate/ITournamentDelegate.hpp"

class TournamentDelegate : public ITournamentDelegate{
public:
    std::string_view CreateTournament(const domain::Tournament& tournament) override {
        //fill groups according to max groups
        for (auto[i, g] = std::tuple{0, 'A'}; i < tournament.Format().NumberOfGroups(); i++,g++) {
            tournament.Groups().push_back(domain::Group{std::format("Tournament {}", g)});
        }

        //if groups are completed also create matches

        return "new-id";
    }
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP