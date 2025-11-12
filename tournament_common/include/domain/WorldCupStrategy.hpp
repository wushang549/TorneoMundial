#pragma once
#include "IMatchStrategy.hpp"

#include <expected>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <string>
#include <iostream>

// String constants for rounds (your Match uses round as string)
namespace rounds {
inline const std::string GROUP = "group";
inline const std::string R16   = "r16";
inline const std::string QF    = "qf";
inline const std::string SF    = "sf";
inline const std::string FINAL = "final";
}

namespace wc {

// Simple standings table (no draws allowed)
struct TableRow {
    std::string teamId;
    std::string teamName;
    int played = 0;
    int won = 0;
    int lost = 0;
    int gf = 0;
    int ga = 0;
    int points = 0;
    int gd() const { return gf - ga; }
};

struct Table {
    std::map<std::string, TableRow> rows; // key: teamId

    void ensureTeam(const std::string& id, const std::string& name) {
        if (!rows.count(id)) rows[id] = TableRow{ id, name };
    }

    // No ties anywhere; winner gets 3 points
    void addMatch(const domain::Match& m) {
        if (!m.HasScore()) return;
        const auto& h  = m.Home();
        const auto& v  = m.Visitor();
        const int sh   = *m.ScoreHome();
        const int sv   = *m.ScoreVisitor();

        ensureTeam(h.Id(), h.Name());
        ensureTeam(v.Id(), v.Name());

        auto& Rh = rows[h.Id()];
        auto& Rv = rows[v.Id()];

        Rh.played++; Rv.played++;
        Rh.gf += sh; Rh.ga += sv;
        Rv.gf += sv; Rv.ga += sh;

        if (sh > sv) { Rh.won++; Rv.lost++; Rh.points += 3; }
        else if (sh < sv) { Rv.won++; Rh.lost++; Rv.points += 3; }
        else {
            // If your backend never stores ties, this path will never hit.
            std::cout << "[WorldCupStrategy] WARNING: tie detected but ties are disallowed\n";
        }
    }

    static bool better(const TableRow& a, const TableRow& b) {
        if (a.points != b.points) return a.points > b.points;
        if (a.gd()   != b.gd())   return a.gd()    > b.gd();
        if (a.gf     != b.gf)     return a.gf      > b.gf;
        return a.teamName < b.teamName;
    }

    std::vector<TableRow> sorted() const {
        std::vector<TableRow> v;
        v.reserve(rows.size());
        for (const auto& kv : rows) v.push_back(kv.second);
        std::sort(v.begin(), v.end(), better);
        return v;
    }
};

} // namespace wc

class WorldCupStrategy : public IMatchStrategy {
public:
    // Round-robin (single) within each group: i<j
    std::expected<std::vector<domain::Match>, std::string>
    CreateRegularPhaseMatches(const domain::Tournament& tournament,
                              const std::vector<std::shared_ptr<domain::Group>>& groups) override
    {
        std::vector<domain::Match> matches;
        if (groups.empty()) return std::unexpected("No groups provided");

        for (const auto& g : groups) {
            const auto& ts = g->Teams();
            if (ts.size() < 3) {
                return std::unexpected("Each group must have at least 3 teams");
            }
            for (size_t i = 0; i < ts.size(); ++i) {
                for (size_t j = i + 1; j < ts.size(); ++j) {
                    domain::Match m;
                    m.TournamentId() = tournament.Id();
                    m.Round()        = rounds::GROUP;
                    m.Home().Id()    = ts[i].Id;
                    m.Home().Name()  = ts[i].Name;
                    m.Visitor().Id() = ts[j].Id;
                    m.Visitor().Name()= ts[j].Name;
                    matches.push_back(m);
                }
            }
        }

        std::cout << "[WorldCupStrategy] Created " << matches.size()
                  << " group-stage matches" << std::endl;
        return matches;
    }

    // R16 from top-2 of paired groups: (A,B), (C,D), ... → A1 vs B2, B1 vs A2
    // Also appends placeholders for QF (4), SF (2), FINAL (1).
    std::expected<std::vector<domain::Match>, std::string>
    CreatePlayoffMatches(const domain::Tournament& tournament,
                         const std::vector<std::shared_ptr<domain::Match>>& regularMatches,
                         const std::vector<std::shared_ptr<domain::Group>>& groups) override
    {
        if (groups.size() % 2 != 0) {
            return std::unexpected("Groups count must be even for pairing");
        }

        // Build per-group tables using only group-stage matches
        std::map<std::string, wc::Table> standingsByGroup;
        for (const auto& g : groups) standingsByGroup[g->Id()] = wc::Table{};

        for (const auto& g : groups) {
            auto& table = standingsByGroup[g->Id()];
            for (const auto& t : g->Teams()) table.ensureTeam(t.Id, t.Name);

            for (const auto& m : regularMatches) {
                if (!m) continue;
                if (m->Round() != rounds::GROUP) continue;

                const auto& hid = m->Home().Id();
                const auto& vid = m->Visitor().Id();
                bool homeIn = false, visIn = false;

                for (const auto& t : g->Teams()) {
                    if (t.Id == hid) homeIn = true;
                    if (t.Id == vid) visIn  = true;
                    if (homeIn && visIn) break;
                }
                if (homeIn && visIn) table.addMatch(*m);
            }
        }

        // Extract top-2 per group
        struct Qualified { std::string id; std::string name; };
        std::map<std::string, std::pair<Qualified, Qualified>> top2;

        for (const auto& g : groups) {
            auto sorted = standingsByGroup[g->Id()].sorted();
            if (sorted.size() < 2) {
                return std::unexpected("Not enough ranked teams in group " + g->Id());
            }
            top2[g->Id()] = {
                Qualified{sorted[0].teamId, sorted[0].teamName},
                Qualified{sorted[1].teamId, sorted[1].teamName}
            };
        }

        // Build R16 pairs by adjacent groups: (0,1), (2,3), ...
        std::vector<domain::Match> ko;
        for (size_t i = 0; i + 1 < groups.size(); i += 2) {
            const auto& gA = groups[i];
            const auto& gB = groups[i + 1];
            const auto A1 = top2[gA->Id()].first;
            const auto A2 = top2[gA->Id()].second;
            const auto B1 = top2[gB->Id()].first;
            const auto B2 = top2[gB->Id()].second;

            // A1 vs B2
            {
                domain::Match m;
                m.TournamentId() = tournament.Id();
                m.Round()        = rounds::R16;
                m.Home().Id()    = A1.id; m.Home().Name()    = A1.name;
                m.Visitor().Id() = B2.id; m.Visitor().Name() = B2.name;
                ko.push_back(m);
            }
            // B1 vs A2
            {
                domain::Match m;
                m.TournamentId() = tournament.Id();
                m.Round()        = rounds::R16;
                m.Home().Id()    = B1.id; m.Home().Name()    = B1.name;
                m.Visitor().Id() = A2.id; m.Visitor().Name() = A2.name;
                ko.push_back(m);
            }
        }

        // Placeholders for next rounds
        for (int i = 0; i < 4; ++i) { domain::Match m; m.TournamentId() = tournament.Id(); m.Round() = rounds::QF;    ko.push_back(m); }
        for (int i = 0; i < 2; ++i) { domain::Match m; m.TournamentId() = tournament.Id(); m.Round() = rounds::SF;    ko.push_back(m); }
        { domain::Match m; m.TournamentId() = tournament.Id(); m.Round() = rounds::FINAL; ko.push_back(m); }

        std::cout << "[WorldCupStrategy] Created " << ko.size()
                  << " knockout matches (incl. placeholders)" << std::endl;
        return ko;
    }
};
