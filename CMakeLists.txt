cmake_minimum_required(VERSION 3.10.0)
project(Tournaments)

cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        activemq-cpp
        URL https://archive.apache.org/dist/activemq/activemq-cpp/3.9.5/activemq-cpp-library-3.9.5-src.tar.gz
)

FetchContent_MakeAvailable(activemq-cpp)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(tournament_common)
add_subdirectory(tournament_services)
add_subdirectory(tournament_consumer)
