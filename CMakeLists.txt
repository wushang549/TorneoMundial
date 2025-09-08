cmake_minimum_required(VERSION 3.10.0)
project(Tournaments)

set(CMAKE_CXX_STANDARD 23)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ProtonCpp CONFIG REQUIRED)

add_subdirectory(tournament_common)
add_subdirectory(tournament_services)
add_subdirectory(tournament_consumer)
