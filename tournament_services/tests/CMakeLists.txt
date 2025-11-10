cmake_minimum_required(VERSION 3.20)
project(tournament_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
find_package(GTest CONFIG REQUIRED)

# Speed toggles
option(MATCHES_ONLY "Build only Match tests" OFF)
option(USE_UNITY_BUILDS_FOR_TESTS "Use Unity builds for test target" ON)

# Test sources (only tests + mocks; no real sources here)
if(MATCHES_ONLY)
    message(STATUS "[tests] MATCHES_ONLY=ON")
    set(TEST_SOURCES
            controller/MatchControllerTest.cpp
            delegate/MatchDelegateTest.cpp
    )
else()
    message(STATUS "[tests] MATCHES_ONLY=OFF (full suite)")
    set(TEST_SOURCES
            controller/TeamControllerTest.cpp
            controller/TournamentControllerTest.cpp
            controller/MatchControllerTest.cpp

            delegate/TeamDelegateTest.cpp
            delegate/TournamentDelegateTest.cpp
            delegate/GroupDelegateTest.cpp
            delegate/MatchDelegateTest.cpp
    )
endif()

add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

# Includes for tests and public headers from the project
target_include_directories(${PROJECT_NAME}_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
        ${CMAKE_CURRENT_SOURCE_DIR}/../include            # tournament_services/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
)

# Link the shared code library (built in parent CMake)
target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gmock
        GTest::gtest_main
        tournament_services_lib          # <-- compile once, reuse here
)

# Unity builds for tests target
if(USE_UNITY_BUILDS_FOR_TESTS)
    set_property(TARGET ${PROJECT_NAME}_runner PROPERTY UNITY_BUILD ON)
endif()

# Reuse PCH from the library if available
# (This requires the lib to define PCH; if not, it's harmless.)
# target_precompile_headers(${PROJECT_NAME}_runner REUSE_FROM tournament_services_lib)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_runner)
