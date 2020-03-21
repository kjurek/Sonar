#include <iostream>
#include <windows.h>

#include "sonar.h"

bool is_home_key_pressed()
{
    return GetAsyncKeyState(VK_HOME) & (1 << 15);
}

bool is_f_key_pressed()
{
    return GetAsyncKeyState(0x46) & (1 << 15);
}

bool is_insert_key_pressed()
{
    return GetAsyncKeyState(VK_INSERT) & (1 << 15);
}

int main()
{
    Sonar sonar;
    while (!is_home_key_pressed()) {
        Sleep(100);
        if (is_insert_key_pressed()) {
            if (sonar.load()) {
                std::cout << "Loaded succesfully" << std::endl;
            } else {
                std::cout << "Could not load" << std::endl;
            }
        }

        if (is_f_key_pressed() && sonar.is_in_game()) {
            try {
                int enemies = sonar.detect_enemies();
                std::cout << "Detected " << enemies << " enemies." << std::endl;
            } catch (ProcessManagerException &e) {
                std::cout << e.what() << std::endl;
            }
        }
    }
}
