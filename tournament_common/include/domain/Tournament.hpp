#ifndef DOMAIN_TOURNAMENT_HPP
#define DOMAIN_TOURNAMENT_HPP

#include <string>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {

    enum class TournamentType {
        ROUND_ROBIN,
        NFL
    };

    class TournamentFormat {
        int numberOfGroups;
        int maxTeamsPerGroup;
        TournamentType type;

    public:
        // World Cup defaults: 8 groups, 4 teams per group
        TournamentFormat(int numberOfGroups = 8,
                         int maxTeamsPerGroup = 4,
                         TournamentType tournamentType = TournamentType::ROUND_ROBIN)
            : numberOfGroups(numberOfGroups),
              maxTeamsPerGroup(maxTeamsPerGroup),
              type(tournamentType) {}

        int NumberOfGroups() const { return numberOfGroups; }
        int& NumberOfGroups() { return numberOfGroups; }

        int MaxTeamsPerGroup() const { return maxTeamsPerGroup; }
        int& MaxTeamsPerGroup() { return maxTeamsPerGroup; }

        TournamentType Type() const { return type; }
        TournamentType& Type() { return type; }
    };

    class Tournament {
        std::string id;
        std::string name;
        TournamentFormat format;
        std::vector<Group> groups;
        std::vector<Match> matches;

    public:
        explicit Tournament(const std::string& name = "",
                            const TournamentFormat& format = TournamentFormat())
            : id(), name(name), format(format), groups(), matches() {}

        std::string Id() const { return id; }
        std::string& Id() { return id; }

        std::string Name() const { return name; }
        std::string& Name() { return name; }

        const TournamentFormat& Format() const { return format; }
        TournamentFormat& Format() { return format; }

        std::vector<Group>& Groups() { return groups; }
        std::vector<Match> Matches() const { return matches; }
    };

} // namespace domain

#endif // DOMAIN_TOURNAMENT_HPP
