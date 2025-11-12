#pragma once
#ifndef TOURNAMENTS_IMATCHSTRATEGY_HPP
#define TOURNAMENTS_IMATCHSTRATEGY_HPP

#include <vector>
#include <memory>
#include <expected>
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"

// Minimal interface used by WorldCupStrategy
class IMatchStrategy {
public:
    virtual ~IMatchStrategy() = default;

    // Create group-stage matches
    virtual std::expected<std::vector<domain::Match>, std::string>
    CreateRegularPhaseMatches(const domain::Tournament& tournament,
                              const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;

    // Create knockout matches (R16 + placeholders for QF/SF/FINAL)
    virtual std::expected<std::vector<domain::Match>, std::string>
    CreatePlayoffMatches(const domain::Tournament& tournament,
                         const std::vector<std::shared_ptr<domain::Match>>& regularMatches,
                         const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;
};

#endif // TOURNAMENTS_IMATCHSTRATEGY_HPP
