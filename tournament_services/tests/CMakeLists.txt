project(tournament_tests)

set(TEST_SOURCES
        # Listener tests
        listener/GroupAddTeamListenerTest.cpp
        listener/MatchCreationListenerTest.cpp
        listener/QueueMessageListenerTest.cpp

        # Controller tests
        controller/TeamControllerTest.cpp
        controller/TournamentControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp

        # Delegate tests
        delegate/TournamentDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp
        delegate/MatchDelegateConsumerTest.cpp

        # Código real que usan los tests
        ../src/controller/GroupController.cpp
        ../src/controller/MatchController.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/TournamentController.cpp
        ../src/delegate/GroupDelegate.cpp
        ../src/delegate/MatchDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
        ../src/delegate/TournamentDelegate.cpp
        listener/MatchCreationListenerTest.cpp
        listener/QueueMessageListenerTest.cpp
        mocks/ConnectionManagerMock.hpp
        listener/ScoreUpdateListenerTest.cpp
        delegate/MatchDelegateConsumerTest.cpp
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
