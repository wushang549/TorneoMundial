# tournament_services/tests/CMakeLists.txt
project(tournament_tests)

set(CMAKE_CXX_STANDARD 23)

set(TEST_SOURCES
        # Tests
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/GroupDelegateTest.cpp

        # Código real que los tests necesitan
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/GroupDelegate.cpp         # 👈 FALTABA: añade GroupDelegate
)

find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

# Includes para que encuentre headers reales y mocks
target_include_directories(${PROJECT_NAME}_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}                 # tests/
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks           # tests/mocks/
        ${CMAKE_CURRENT_SOURCE_DIR}/../include      # tournament_services/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
)

# Enlaces: libs usadas por controllers/delegates
target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        tournament_common            # tu lib común (repos, dominio, etc.)

        # Si controladores usan Crow/asio/json:
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json

        # pqxx/postgres si algo del código real lo usa en build de tests
        libpqxx::pqxx
)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)
