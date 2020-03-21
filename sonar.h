#pragma once

#include "process_manager.h"
#include <vector>

struct Player;

class Sonar
{
public:
    bool load();
    bool is_in_game() const;
    int detect_enemies() const;

private:
    DWORD get_client_address() const;
    DWORD get_engine_address() const;
    Player get_player(DWORD address) const;

private:
    ProcessManager _manager;
};
