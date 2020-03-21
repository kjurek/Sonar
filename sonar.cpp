#include "sonar.h"
#include "csgo.hpp"
#include "player.h"

bool Sonar::load()
{
    try {
        _manager.configure_from_process_name("csgo.exe");
        return true;
    } catch (ProcessManagerException &e) {
        return false;
    }
}

bool Sonar::is_in_game() const
{
    if (!_manager.is_configured())
        return false;

    DWORD client_state_address = _manager.read_memory<DWORD>(
        get_engine_address() + hazedumper::signatures::dwClientState);
    DWORD client_state = _manager.read_memory<DWORD>(client_state_address
                                                     + hazedumper::signatures::dwClientState_State);
    return client_state;
}

int Sonar::detect_enemies() const
{
    DWORD local_player_address = _manager.read_memory<DWORD>(
        get_client_address() + hazedumper::signatures::dwLocalPlayer);
    Player local_player = get_player(local_player_address);

    DWORD player_list_address = get_client_address() + hazedumper::signatures::dwEntityList;
    const int MAX_NUMBER_OF_PLAYERS = 65;

    int detected_enemies = 0;
    for (int i = 1; i <= MAX_NUMBER_OF_PLAYERS; ++i) {
        DWORD other_player_address = _manager.read_memory<DWORD>(player_list_address + i * 0x10);
        if (other_player_address == 0)
            continue;

        Player p = get_player(other_player_address);
        if (p.hp > 0 && p.team != local_player.team) {
            _manager.write_memory<int>(other_player_address + hazedumper::netvars::m_bSpotted, 1);
            ++detected_enemies;
        }
    }
    return detected_enemies;
}

DWORD Sonar::get_client_address() const
{
    return (DWORD) _manager.get_module_address("client_panorama.dll");
}

DWORD Sonar::get_engine_address() const
{
    return (DWORD) _manager.get_module_address("engine.dll");
}

Player Sonar::get_player(DWORD address) const
{
    Player p;
    p.team = _manager.read_memory<Team>(address + hazedumper::netvars::m_iTeamNum);
    p.pos = _manager.read_memory<float>(address + hazedumper::netvars::m_vecOrigin, 3);
    p.hp = _manager.read_memory<int>(address + hazedumper::netvars::m_iHealth);
    return p;
}
