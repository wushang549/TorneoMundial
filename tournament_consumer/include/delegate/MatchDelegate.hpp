#pragma once
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include "event/TeamAddEvent.hpp"
#include "event/ScoreUpdateEvent.hpp"

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

#include "domain/Match.hpp"
#include "domain/WorldCupStrategy.hpp"

class MatchDelegate {
    std::shared_ptr<IMatchRepository>     matchRepository;
    std::shared_ptr<IGroupRepository>     groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<IGroupRepository>& groupRepository,
                  const std::shared_ptr<TournamentRepository>& tournamentRepository)
        : matchRepository(matchRepository),
          groupRepository(groupRepository),
          tournamentRepository(tournamentRepository) {}

    void ProcessTeamAddition(const TeamAddEvent& teamAddEvent) {
        std::cout << "[MatchDelegate/WC] Team added in tournament: "
                  << teamAddEvent.tournamentId << "\n";

        // Group-level progress (affected group)
        if (auto grp = groupRepository->FindByTournamentIdAndGroupId(
                teamAddEvent.tournamentId, teamAddEvent.groupId)) {
            auto tRes = tournamentRepository->ReadById(teamAddEvent.tournamentId);
            int expectedPerGroup = tRes ? tRes->Format().MaxTeamsPerGroup() : 0;
            std::cout << "[WC] Group " << grp->Name() << ": "
                      << grp->Teams().size() << "/" << expectedPerGroup
                      << " teams\n";
        }

        // Global tournament progress
        {
            auto allGroups = groupRepository->FindByTournamentId(teamAddEvent.tournamentId);
            if (!allGroups.empty()) {
                auto tRes = tournamentRepository->ReadById(teamAddEvent.tournamentId);
                int expectedGroups   = tRes ? tRes->Format().NumberOfGroups()   : 0;
                int expectedPerGroup = tRes ? tRes->Format().MaxTeamsPerGroup() : 0;

                int complete = 0;
                for (const auto& g : allGroups) {
                    if ((int)g->Teams().size() == expectedPerGroup) complete++;
                }
                std::cout << "[WC] Progress: " << complete << "/" << expectedGroups
                          << " groups complete\n";
            }
        }

        if (IsTournamentReady(teamAddEvent.tournamentId)) {
            std::cout << "[MatchDelegate/WC] All groups complete -> creating group-stage matches...\n";
            CreateGroupStageMatches(teamAddEvent.tournamentId);
        } else {
            std::cout << "[MatchDelegate/WC] Tournament not ready yet; waiting for more teams...\n";
        }
    }

    void ProcessScoreUpdate(const ScoreUpdateEvent& scoreUpdateEvent) {
        std::cout << "[MatchDelegate/WC] Score update for tournament: "
                  << scoreUpdateEvent.tournamentId << "\n";

        if (AllGroupMatchesPlayed(scoreUpdateEvent.tournamentId)) {
            if (!IsInKnockouts(scoreUpdateEvent.tournamentId)) {
                std::cout << "[MatchDelegate/WC] Group stage complete -> creating knockouts...\n";
                CreateKnockoutMatches(scoreUpdateEvent.tournamentId);
            } else {
                std::cout << "[MatchDelegate/WC] Already in knockouts (advance wiring optional).\n";
            }
        } else {
            std::cout << "[MatchDelegate/WC] Still pending group matches...\n";
        }
    }

private:
    bool IsTournamentReady(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] Tournament not found\n";
            return false;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        const int expectedGroups = t->Format().NumberOfGroups();
        if (static_cast<int>(groups.size()) != expectedGroups) {
            std::cout << "[WC] Groups count mismatch. Have " << groups.size()
                      << " expected " << expectedGroups << "\n";
            return false;
        }

        const int teamsPerGroup = t->Format().MaxTeamsPerGroup();
        for (const auto& g : groups) {
            if (static_cast<int>(g->Teams().size()) != teamsPerGroup) {
                std::cout << "[WC] Group " << g->Name() << " has "
                          << g->Teams().size() << " teams (expected "
                          << teamsPerGroup << ")\n";
                return false;
            }
        }
        return true;
    }

    bool AllGroupMatchesPlayed(const std::string& tournamentId) {
        auto all = matchRepository->FindByTournamentId(tournamentId);
        for (const auto& m : all) {
            if (!m) continue;
            if (m->Round() == rounds::GROUP && (m->Status() == "pending" || !m->HasScore()))
                return false;
        }
        return true;
    }

    bool IsInKnockouts(const std::string& tournamentId) {
        auto all = matchRepository->FindByTournamentId(tournamentId);
        return std::any_of(all.begin(), all.end(),
                           [](const std::shared_ptr<domain::Match>& m) {
                               return m && m->Round() == rounds::R16;
                           });
    }

    void CreateGroupStageMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] ERROR: tournament not found\n";
            return;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);

        WorldCupStrategy s;
        auto createdOrErr = s.CreateRegularPhaseMatches(*t, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << "\n";
            return;
        }
        const auto& created = createdOrErr.value();

        int ok = 0;
        for (const auto& m : created) {
            const std::string id = matchRepository->Create(m);
            if (!id.empty()) ok++;
            else std::cout << "[WC] ERROR creating match\n";
        }
        std::cout << "[WC] Created " << ok << "/" << created.size()
                  << " group matches\n";
    }

    void CreateKnockoutMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) {
            std::cout << "[WC] ERROR: tournament not found\n";
            return;
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        auto all    = matchRepository->FindByTournamentId(tournamentId);

        WorldCupStrategy s;
        auto createdOrErr = s.CreatePlayoffMatches(*t, all, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << "\n";
            return;
        }
        const auto& created = createdOrErr.value();

        int ok = 0;
        std::vector<std::string> ids;
        std::vector<std::shared_ptr<domain::Match>> saved;

        for (const auto& m : created) {
            const std::string id = matchRepository->Create(m);
            if (!id.empty()) {
                ok++;
                ids.push_back(id);
                saved.push_back(
                    matchRepository->FindByTournamentIdAndMatchId(tournamentId, id)
                );
            } else {
                std::cout << "[WC] ERROR creating KO match\n";
            }
        }

        // Optional: wire bracket links here if you decide to keep explicit next-match pointers.

        std::cout << "[WC] Created " << ok << "/" << created.size()
                  << " knockout matches\n";
    }
};
