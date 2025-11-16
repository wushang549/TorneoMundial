#pragma once
#include "IMatchStrategy.hpp"

#include <expected>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <string>
#include <optional>
#include <iostream>

// Round keys
namespace rounds {
inline const std::string GROUP = "group";
inline const std::string R16   = "r16";
inline const std::string QF    = "qf";
inline const std::string SF    = "sf";
inline const std::string FINAL = "final";
}

namespace wc {

// Basic standings table
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
            // No ties expected in your flow
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
    struct TeamRef {
        std::string id;
        std::string name;
    };
    struct Pair { TeamRef home; TeamRef visitor; };

    static std::optional<const domain::Match*> findMatch(
        const std::vector<std::shared_ptr<domain::Match>>& all,
        const std::string& roundKey,
        const std::string& homeId,
        const std::string& visId)
    {
        for (const auto& sp : all) {
            if (!sp) continue;
            const auto& m = *sp;
            if (m.Round() != roundKey) continue;
            if (m.Home().Id() == homeId && m.Visitor().Id() == visId) {
                return std::optional<const domain::Match*>{ &m };
            }
        }
        return std::nullopt;
    }

    static std::optional<TeamRef> winnerOf(const domain::Match& m) {
        if (!m.HasScore()) return std::nullopt;
        const int sh = *m.ScoreHome();
        const int sv = *m.ScoreVisitor();
        if (sh == sv) return std::nullopt; // no draws expected
        if (sh > sv)  return TeamRef{ m.Home().Id(), m.Home().Name() };
        return TeamRef{ m.Visitor().Id(), m.Visitor().Name() };
    }

public:
    // Group round-robin (unchanged)
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

    // NEW: Progressive KO generation (R16 -> QF -> SF -> FINAL) without placeholders
    std::expected<std::vector<domain::Match>, std::string>
    CreatePlayoffMatches(const domain::Tournament& tournament,
                         const std::vector<std::shared_ptr<domain::Match>>& allMatches,
                         const std::vector<std::shared_ptr<domain::Group>>& groups) override
    {
        if (groups.size() % 2 != 0) {
            return std::unexpected("Groups count must be even for pairing");
        }

        // --- 1) Build tables from GROUP matches only ---
        std::map<std::string, wc::Table> standingsByGroup;
        for (const auto& g : groups) standingsByGroup[g->Id()] = wc::Table{};

        for (const auto& g : groups) {
            auto& table = standingsByGroup[g->Id()];
            for (const auto& t : g->Teams()) table.ensureTeam(t.Id, t.Name);
        }
        for (const auto& msp : allMatches) {
            if (!msp) continue;
            const auto& m = *msp;
            if (m.Round() != rounds::GROUP) continue;
            // Identify the group this match belongs to (both teams in same group)
            for (const auto& g : groups) {
                bool homeIn=false, visIn=false;
                for (const auto& t : g->Teams()) {
                    if (t.Id == m.Home().Id()) homeIn = true;
                    if (t.Id == m.Visitor().Id()) visIn = true;
                    if (homeIn && visIn) break;
                }
                if (homeIn && visIn) {
                    standingsByGroup[g->Id()].addMatch(m);
                    break;
                }
            }
        }

        // --- 2) Get top-2 per group ---
        struct Qualified { TeamRef first; TeamRef second; };
        std::map<std::string, Qualified> top2;
        for (const auto& g : groups) {
            auto sorted = standingsByGroup[g->Id()].sorted();
            if (sorted.size() < 2) {
                return std::unexpected("Not enough ranked teams in group " + g->Id());
            }
            top2[g->Id()] = Qualified{
                TeamRef{sorted[0].teamId, sorted[0].teamName},
                TeamRef{sorted[1].teamId, sorted[1].teamName}
            };
        }

        // Helper: append a KO match to out vector
        auto appendMatch = [&](std::vector<domain::Match>& out,
                               const std::string& roundKey,
                               const TeamRef& H, const TeamRef& V)
        {
            domain::Match m;
            m.TournamentId() = tournament.Id();
            m.Round()        = roundKey;
            m.Home().Id()    = H.id; m.Home().Name()    = H.name;
            m.Visitor().Id() = V.id; m.Visitor().Name() = V.name;
            out.push_back(std::move(m));
        };

        std::vector<domain::Match> result;

        // --- 3) R16 pairs in deterministic order: (A,B), (C,D), ... ---
        std::vector<Pair> r16Pairs;
        r16Pairs.reserve(16);
        for (size_t i = 0; i + 1 < groups.size(); i += 2) {
            const auto& gA = groups[i];
            const auto& gB = groups[i + 1];
            const auto A1 = top2[gA->Id()].first;
            const auto A2 = top2[gA->Id()].second;
            const auto B1 = top2[gB->Id()].first;
            const auto B2 = top2[gB->Id()].second;

            r16Pairs.push_back(Pair{ A1, B2 }); // A1 vs B2
            r16Pairs.push_back(Pair{ B1, A2 }); // B1 vs A2
        }

        // Create R16 (only if not already persisted; MatchDelegate deduplica antes de insertar)
        for (const auto& p : r16Pairs) {
            // If this exact R16 already exists, delegate will skip; we still propose it here.
            appendMatch(result, rounds::R16, p.home, p.visitor);
        }

        // --- 4) If R16 played, compose QF in same pairing order (0-1, 2-3, 4-5, 6-7) ---
        std::vector<TeamRef> r16Winners;
        r16Winners.reserve(r16Pairs.size());
        bool r16Complete = true;
        for (const auto& p : r16Pairs) {
            auto mOpt = findMatch(allMatches, rounds::R16, p.home.id, p.visitor.id);
            if (!mOpt.has_value()) { r16Complete = false; break; }
            auto w = winnerOf(**mOpt);
            if (!w.has_value()) { r16Complete = false; break; }
            r16Winners.push_back(*w);
        }
        if (r16Complete && r16Winners.size() == 8) {
            // QF: winners (0 vs 1), (2 vs 3), (4 vs 5), (6 vs 7)
            for (int i = 0; i < 8; i += 2) {
                appendMatch(result, rounds::QF, r16Winners[i], r16Winners[i+1]);
            }
        }

        // --- 5) If QF played, compose SF (0-1, 2-3) ---
        std::vector<Pair> qfPairs;
        if (r16Complete) {
            for (int i = 0; i < 8; i += 2) {
                qfPairs.push_back(Pair{ r16Winners[i], r16Winners[i+1] });
            }
        }

        std::vector<TeamRef> qfWinners;
        bool qfComplete = false;
        if (!qfPairs.empty()) {
            qfWinners.reserve(4);
            qfComplete = true;
            for (const auto& p : qfPairs) {
                auto mOpt = findMatch(allMatches, rounds::QF, p.home.id, p.visitor.id);
                if (!mOpt.has_value()) { qfComplete = false; break; }
                auto w = winnerOf(**mOpt);
                if (!w.has_value()) { qfComplete = false; break; }
                qfWinners.push_back(*w);
            }
            if (qfComplete && qfWinners.size() == 4) {
                appendMatch(result, rounds::SF, qfWinners[0], qfWinners[1]);
                appendMatch(result, rounds::SF, qfWinners[2], qfWinners[3]);
            }
        }

        // --- 6) If SF played, compose FINAL ---
        std::vector<Pair> sfPairs;
        if (qfComplete) {
            sfPairs.push_back(Pair{ qfWinners[0], qfWinners[1] });
            sfPairs.push_back(Pair{ qfWinners[2], qfWinners[3] });
        }

        bool sfComplete = false;
        if (!sfPairs.empty()) {
            std::vector<TeamRef> sfWinners;
            sfWinners.reserve(2);
            sfComplete = true;
            for (const auto& p : sfPairs) {
                auto mOpt = findMatch(allMatches, rounds::SF, p.home.id, p.visitor.id);
                if (!mOpt.has_value()) { sfComplete = false; break; }
                auto w = winnerOf(**mOpt);
                if (!w.has_value()) { sfComplete = false; break; }
                sfWinners.push_back(*w);
            }
            if (sfComplete && sfWinners.size() == 2) {
                appendMatch(result, rounds::FINAL, sfWinners[0], sfWinners[1]);
            }
        }

        std::cout << "[WorldCupStrategy] KO proposals: "
                  << result.size() << " (R16/QF/SF/FINAL as available)" << std::endl;
        return result;
    }
};
