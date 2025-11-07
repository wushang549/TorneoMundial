#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <expected>     // C++23 <expected>
#include "domain/Match.hpp"

// Forward declarations to avoid heavy includes at header level.
// Implementations are included in the .cpp.
class IMatchRepository;
class ITournamentDelegate;

/**
 * Public-facing interface for the match business logic.
 * Controllers should depend on this interface, not on the repository directly.
 */
class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    /**
     * Returns all matches for a tournament.
     * If showFilter is "played" or "pending", the result is filtered accordingly.
     * Throws (mapped by controller) if tournament doesn't exist.
     */
    virtual std::vector<std::shared_ptr<domain::Match>>
    ReadAll(const std::string& tournamentId,
            const std::optional<std::string_view>& showFilter) = 0;

    /**
     * Returns a single match for a given tournament and match id.
     * Returns nullptr if not found (controller maps to 404).
     */
    virtual std::shared_ptr<domain::Match>
    ReadById(const std::string& tournamentId, const std::string& matchId) = 0;

    /**
     * Updates the score for a match.
     * Business rules:
     *  - Score bounds: 0..10 (both sides)
     *  - No draws allowed; if equal, a deterministic tie-break selects a winner.
     *
     * Returns:
     *   value()                         -> success
     *   unexpected("not_found")         -> controller maps to 404
     *   unexpected("validation:<reason>")-> controller maps to 422
     *   unexpected("unexpected:<...>")  -> controller maps to 500
     */
    virtual std::expected<void, std::string>
    UpdateScore(const std::string& tournamentId,
                const std::string& matchId,
                int homeScore, int visitorScore) = 0;
};

/**
 * Concrete implementation of IMatchDelegate.
 * Encapsulates tournament existence checks and match rules (no draws).
 */
class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IMatchRepository> matchRepository;
    std::shared_ptr<ITournamentDelegate> tournamentDelegate;

    /**
     * Deterministic tie-breaker to keep idempotency across retries.
     * Seed is derived from immutable identifiers to produce the same winner
     * every time for the same (tournament, match).
     */
    static std::string pickDeterministicWinner(const std::string& tournamentId,
                                               const std::string& matchId,
                                               const std::string& homeTeamId,
                                               const std::string& visitorTeamId);
public:
    MatchDelegate(std::shared_ptr<IMatchRepository> matchRepo,
                  std::shared_ptr<ITournamentDelegate> tournamentDel);

    std::vector<std::shared_ptr<domain::Match>>
    ReadAll(const std::string& tournamentId,
            const std::optional<std::string_view>& showFilter) override;

    std::shared_ptr<domain::Match>
    ReadById(const std::string& tournamentId, const std::string& matchId) override;

    std::expected<void, std::string>
    UpdateScore(const std::string& tournamentId,
                const std::string& matchId,
                int homeScore, int visitorScore) override;
};
