project(tournament_tests)

set(TEST_SOURCES
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp
        delegate/TournamentDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        ../src/controller/GroupController.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../include/controller/GroupController.hpp
        ../include/controller/MatchController.hpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
        ../include/delegate/GroupDelegate.hpp
        ../../tournament_common/src/persistence/repository/MatchRepository.cpp
        ../../tournament_consumer/include/cms/MatchCreationListener.hpp
        ../src/delegate/MatchDelegate.cpp
        ../src/controller/MatchController.cpp
)

set(SOURCES ${TEST_SOURCES})
include_directories(
        ../include
        ../../tournament_consumer/include
        ./        # <-- agrega este
        ./mocks   # <-- agrega este
)
find_package(GTest CONFIG REQUIRED)



add_executable(${PROJECT_NAME}_runner
        ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        Crow::Crow
        unofficial::activemq-cpp::activemq-cpp
        tournament_common)


add_test(AllTestsInMain ${PROJECT_NAME}_runner)
