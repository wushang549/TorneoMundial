//IMatchDelegate.hpp
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <expected>
#include <nlohmann/json.hpp>

namespace domain { class Match; }

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    virtual std::vector<std::shared_ptr<domain::Match>>
    ReadAll(const std::string& tournamentId,
            const std::optional<std::string_view>& showFilter) = 0;

    virtual std::shared_ptr<domain::Match>
    ReadById(const std::string& tournamentId, const std::string& matchId) = 0;

    virtual std::expected<void, std::string>
    UpdateScore(const std::string& tournamentId, const std::string& matchId,
                int home, int visitor) = 0;

    // Create endpoint support
    virtual std::expected<std::string, std::string>
    Create(const std::string& tournamentId, const nlohmann::json& body) = 0;
};
