cmake_minimum_required(VERSION 3.22)
project(tournament_services)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Speed toggles
option(USE_UNITY_BUILDS "Use Unity builds for services" ON)

# ---- sources (shared by exe and tests through a static lib) ----
set(SERVICES_SOURCES
        # Delegates
        src/delegate/TeamDelegate.cpp
        src/delegate/TournamentDelegate.cpp
        src/delegate/GroupDelegate.cpp
        src/delegate/MatchDelegate.cpp

        # Controllers
        src/controller/TournamentController.cpp
        src/controller/TeamController.cpp
        src/controller/GroupController.cpp
        src/controller/MatchController.cpp
)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")

# Optional asio target (some Crow packages export it, some don't)
# We'll link it only if exists downstream.

# ---- build a static library to avoid recompiling sources in tests ----
add_library(tournament_services_lib STATIC ${SERVICES_SOURCES})

target_include_directories(tournament_services_lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
        ${HYPODERMIC_INCLUDE_DIRS}
)

# Link dependencies PUBLIC so tests inherit them transitively
target_link_libraries(tournament_services_lib
        PUBLIC
        Crow::Crow
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        tournament_common
        Threads::Threads
)

# Link asio only if available as a target
if(TARGET asio::asio)
    target_link_libraries(tournament_services_lib PUBLIC asio::asio)
endif()

# Do NOT link ActiveMQ here; it's for the consumer
# if(TARGET unofficial::activemq-cpp::activemq-cpp)
#   target_link_libraries(tournament_services_lib PUBLIC unofficial::activemq-cpp::activemq-cpp)
# endif()

# Unity builds + PCH to speed up Crow/JSON heavy templates
if(USE_UNITY_BUILDS)
    set_property(TARGET tournament_services_lib PROPERTY UNITY_BUILD ON)
endif()

# Precompiled headers (safe to include, adjust if your include path differs)
target_precompile_headers(tournament_services_lib PRIVATE
        <crow.h>
        <nlohmann/json.hpp>
)

# ---- main executable (very thin, reuses the lib) ----
add_executable(${PROJECT_NAME}
        main.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
        ${HYPODERMIC_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        tournament_services_lib
)

# Runtime config next to the binary
configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
        COPYONLY
)

# ---- tests subdirectory (now that the lib exists) ----
add_subdirectory(tests)
