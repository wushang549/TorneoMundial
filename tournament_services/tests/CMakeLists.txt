# tournament_services/tests/CMakeLists.txt
project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/TournamentDelegateTest.cpp           # <-- este nuevo
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        mocks/GroupRepositoryMock.hpp
        # <-- SUT TournamentDelegate
)

find_package(GTest CONFIG REQUIRED)
# Si no heredas del padre, descomenta estas:
# find_package(Crow CONFIG REQUIRED)
# find_package(nlohmann_json CONFIG REQUIRED)
# find_package(asio CONFIG REQUIRED) # solo si realmente lo necesitas como target

add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

target_include_directories(${PROJECT_NAME}_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        Crow::Crow
        nlohmann_json::nlohmann_json
        # asio::asio                            # solo si lo necesitas explícitamente
        tournament_common
)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)
