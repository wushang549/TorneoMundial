# tournament_services/tests/CMakeLists.txt
project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        # No metas .cpp de mocks (son headers-only) ni dupliques headers en SOURCES
        # Si necesitas compilar los controllers reales para probarlos, añádelos aquí:
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
)

find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

# Rutas de include que harán que #include "mocks/TeamDelegateMock.hpp" funcione
target_include_directories(${PROJECT_NAME}_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}            # tests/
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks      # tests/mocks/
        ${CMAKE_CURRENT_SOURCE_DIR}/../include # tournament_services/include (tus headers reales)
        ${CMAKE_SOURCE_DIR}/tournament_common/include
)

# Enlace: solo un *main* (gtest_main o gmock_main, no ambos)
target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        tournament_common

        # Si los controllers incluyen Crow/asio/json (común):
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)
