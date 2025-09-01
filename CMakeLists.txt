cmake_minimum_required(VERSION 3.10.0)
project(Tournaments)

set(CMAKE_CXX_STANDARD 23)
set(SOURCES 
        src/delegate/TeamDelegate.cpp
        src/controller/TournamentController.cpp)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(amqpcpp CONFIG REQUIRED)

include_directories(include)

add_subdirectory(tests)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        amqpcpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${HYPODERMIC_INCLUDE_DIRS})

configure_file(
    ${CMAKE_SOURCE_DIR}/configuration.json   # source file
    ${CMAKE_BINARY_DIR}/configuration.json  # destination
    COPYONLY
)     