find_package(GTest CONFIG REQUIRED)

include_directories(include)

add_executable(team_controller_runner
    TeamControllerTest.cpp
)

target_link_libraries(team_controller_runner PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

add_test(AllTestsInMain team_controller_runner)