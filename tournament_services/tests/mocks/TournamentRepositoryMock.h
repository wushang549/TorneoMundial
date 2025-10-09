#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

// Ajusta esta ruta si tu header vive en otro namespace/carpeta:
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"

// Si IRepository está en un namespace, ajusta esta línea, por ejemplo:
// using TournamentRepoIface = persistence::repository::IRepository<domain::Tournament, std::string>;
using TournamentRepoIface = IRepository<domain::Tournament, std::string>;

class MockTournamentRepository : public TournamentRepoIface {
public:
    // Create devuelve Id (std::string)
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));

    // Read
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));

    // Update devuelve Id (std::string)
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));

    // Delete recibe Id (std::string) y retorna void
    MOCK_METHOD(void, Delete, (std::string), (override));
};
