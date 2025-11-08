cmake_minimum_required(VERSION 3.22)
project(tournament_services)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- sources ----
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

add_subdirectory(tests)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SERVICES_SOURCES}
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${HYPODERMIC_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common
        Threads::Threads
)

# Copy runtime config next to the binary
configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
        COPYONLY
)
