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
                  << teamAddEvent.tournamentId << std::endl;

        if (IsTournamentReady(teamAddEvent.tournamentId)) {
            std::cout << "[MatchDelegate/WC] All groups complete → creating group-stage matches..."
                      << std::endl;
            CreateGroupStageMatches(teamAddEvent.tournamentId);
        } else {
            std::cout << "[MatchDelegate/WC] Tournament not ready yet; waiting for more teams..."
                      << std::endl;
        }
    }

    void ProcessScoreUpdate(const ScoreUpdateEvent& scoreUpdateEvent) {
        std::cout << "[MatchDelegate/WC] Score update for tournament: "
                  << scoreUpdateEvent.tournamentId << std::endl;

        if (AllGroupMatchesPlayed(scoreUpdateEvent.tournamentId)) {
            if (!IsInKnockouts(scoreUpdateEvent.tournamentId)) {
                std::cout << "[MatchDelegate/WC] Group stage complete → creating knockouts..."
                          << std::endl;
                CreateKnockoutMatches(scoreUpdateEvent.tournamentId);
            } else {
                std::cout << "[MatchDelegate/WC] Already in knockouts (advance wiring optional)."
                          << std::endl;
                // Optional: AdvanceKnockoutMatch(scoreUpdateEvent.matchId);
            }
        } else {
            std::cout << "[MatchDelegate/WC] Still pending group matches..." << std::endl;
        }
    }

private:
    bool IsTournamentReady(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) { std::cout << "[WC] Tournament not found\n"; return false; }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        const int expectedGroups = t->Format().NumberOfGroups();
        if (static_cast<int>(groups.size()) != expectedGroups) {
            std::cout << "[WC] Groups count mismatch. Have " << groups.size()
                      << " expected " << expectedGroups << std::endl;
            return false;
        }

        const int teamsPerGroup = t->Format().MaxTeamsPerGroup();
        for (auto& g : groups) {
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
        for (auto& m : all) {
            if (!m) continue;
            if (m->Round() == rounds::GROUP && m->Status() == "pending")
                return false;
            if (m->Round() == rounds::GROUP && !m->HasScore())
                return false;
        }
        return true;
    }

    bool IsInKnockouts(const std::string& tournamentId) {
        auto all = matchRepository->FindByTournamentId(tournamentId);
        return std::any_of(all.begin(), all.end(), [](const std::shared_ptr<domain::Match>& m){
            return m && m->Round() == rounds::R16;
        });
    }

    void CreateGroupStageMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) { std::cout << "[WC] ERROR: tournament not found\n"; return; }

        auto groups = groupRepository->FindByTournamentId(tournamentId);

        WorldCupStrategy s;
        auto createdOrErr = s.CreateRegularPhaseMatches(*t, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << std::endl;
            return;
        }
        const auto& created = createdOrErr.value();

        int ok = 0;
        for (const auto& m : created) {
            const std::string id = matchRepository->Create(m); // by const-ref
            if (!id.empty()) ok++;
            else std::cout << "[WC] ERROR creating match\n";
        }
        std::cout << "[WC] Created " << ok << "/" << created.size()
                  << " group matches" << std::endl;
    }

    void CreateKnockoutMatches(const std::string& tournamentId) {
        auto t = tournamentRepository->ReadById(tournamentId);
        if (!t) { std::cout << "[WC] ERROR: tournament not found\n"; return; }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        auto all    = matchRepository->FindByTournamentId(tournamentId);

        WorldCupStrategy s;
        auto createdOrErr = s.CreatePlayoffMatches(*t, all, groups);
        if (!createdOrErr) {
            std::cout << "[WC] Strategy error: " << createdOrErr.error() << std::endl;
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
                saved.push_back(matchRepository->FindByTournamentIdAndMatchId(tournamentId, id));
            } else {
                std::cout << "[WC] ERROR creating KO match\n";
            }
        }

        // Optional: wire QF/SF/FINAL here (define your bracket indices if needed)
        // saved[idx]->SetNextMatchId(ids[nextIdx]);
        // saved[idx]->SetNextMatchWinnerSlot("home"/"visitor");
        // matchRepository->Update(*saved[idx]);

        std::cout << "[WC] Created " << ok << "/" << created.size()
                  << " knockout matches" << std::endl;
    }
};
