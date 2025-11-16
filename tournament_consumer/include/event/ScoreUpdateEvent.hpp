#ifndef TOURNAMENTS_SCOREUPDATEEVENT_HPP
#define TOURNAMENTS_SCOREUPDATEEVENT_HPP
#include <string>

struct ScoreUpdateEvent {
    std::string tournamentId;
    std::string matchId;
};
#endif //TOURNAMENTS_SCOREUPDATEEVENT_HPP