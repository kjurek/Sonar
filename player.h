#pragma once

#include <vector>

enum Team {
    TERRORIST = 2,
    COUNTER_TERRORIST = 3
};

struct Player
{
    Team team;
    std::vector<float> pos;
    int hp;
};
