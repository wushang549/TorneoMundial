#pragma once
#include <expected>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include "delegate/ITournamentDelegate.hpp"
#include "domain/Tournament.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD( (std::expected<std::string, std::string>),
                 CreateTournament,
                 (std::shared_ptr<domain::Tournament>),
                 (override) );

    MOCK_METHOD( (std::expected<std::vector<std::shared_ptr<domain::Tournament>>, std::string>),
                 ReadAll,
                 (),
                 (override) );

    MOCK_METHOD( (std::expected<std::shared_ptr<domain::Tournament>, std::string>),
                 ReadById,
                 (const std::string&),
                 (override) );

    MOCK_METHOD( (std::expected<bool, std::string>),
                 UpdateTournament,
                 (const std::string&, const domain::Tournament&),
                 (override) );

    MOCK_METHOD( (std::expected<bool, std::string>),
                 DeleteTournament,
                 (const std::string&),
                 (override) );
};
