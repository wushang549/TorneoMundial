#include <gtest/gtest.h>
#include "domain/Team.hpp"


TEST(TournamentControllerTest, CreateTournament) {
    std::string id = "ID";
    std::string name = "Name";

    domain::Team team = {id,name};    

    EXPECT_EQ(team.Id.c_str(), id);
    EXPECT_EQ(team.Name.c_str(), name);
}