project(tournament_tests)

set(TEST_SOURCES 
    TeamControllerTest.cpp
    TournamentControllerTest.cpp)

set(SOURCES ${SOURCES} ${TEST_SOURCES})

find_package(GTest CONFIG REQUIRED)

include_directories(include)

add_executable(${PROJECT_NAME}_runner
    ${TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(AllTestsInMain ${PROJECT_NAME}_runner)