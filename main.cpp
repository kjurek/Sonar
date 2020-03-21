#include <iostream>
#include <windows.h>

#include "sonar.h"

bool isHomeKeyPressed()
{
    return GetAsyncKeyState(VK_HOME) & (1 << 15);
}

bool isFKeyPressed()
{
    return GetAsyncKeyState(0x46) & (1 << 15);
}

bool isInsertKeyPressed()
{
    return GetAsyncKeyState(VK_INSERT) & (1 << 15);
}

int main()
{
    Sonar sonar;
    while (!isHomeKeyPressed())
    {
        Sleep(100);
        if (isInsertKeyPressed())
        {
            if (sonar.load())
            {
                std::cout << "Loaded succesfully" << std::endl;
            }
            else
            {
                std::cout << "Could not load" << std::endl;
            }
        }

        if (isFKeyPressed() && sonar.is_in_game())
        {
            try
            {
                int enemies = sonar.detect_enemies();
                std::cout << "Detected " << enemies << " enemies." << std::endl;
            }
            catch (ProcessManagerException& e)
            {
                std::cout << e.what() << std::endl;
            }
        }
    }
}
