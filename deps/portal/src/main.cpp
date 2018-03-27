/*
portal
Copyright (C) 2018	Will Townsend <will@townsend.io>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h>

#include "Portal.hpp"

bool m_running = true;

void sig_interrupt(int sig)
{
    m_running = false;
}

int main()
{

    std::cout << "Looking for devices" << std::endl;
    std::shared_ptr<portal::Portal> client = std::make_shared<portal::Portal>();

    client->startListeningForDevices();
    signal(SIGINT, sig_interrupt);

    while (m_running && client->isListening())
    {
        //waiting for device
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Done!" << std::endl;

    return 0;
}
