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

#include <cstdint>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#endif

#include "Protocol.hpp"

namespace portal
{

SimpleDataPacketProtocol::SimpleDataPacketProtocol()
{
    std::cout << "SimpleDataPacketProtocol created\n";
}

SimpleDataPacketProtocol::~SimpleDataPacketProtocol()
{
    buffer.clear();
    std::cout << "SimpleDataPacketProtocol destroyed\n";
}

int SimpleDataPacketProtocol::processData(char *data, int dataLength)
{

    printf("%s: Init\n", __func__);

    if (this == nullptr)
    {
        printf("For some reason the simple data packet protocol doesn't exist");
        return -1;
    }

    if (dataLength > 0)
    {
        // Add data recieved to the end of buffer.
        buffer.insert(buffer.end(), data, data + dataLength);
    }

    uint32_t length = 0;
    // Ensure that the data inside the buffer is at least as big
    // as the length variable (32 bit int)
    // and then read it out
    if (buffer.size() < sizeof(length))
    {
        return -1;
    }
    else
    {

        // Grab the length value out
        memcpy(&length, &buffer[0], sizeof length);
        length = ntohl(length);

        while (sizeof(length) + length <= buffer.size())
        {
            // Read length bytes as that is the packet
            std::vector<char>::const_iterator first = buffer.begin() + sizeof(length);
            std::vector<char>::const_iterator last = buffer.begin() + sizeof(length) + length;
            std::vector<char> newVec(first, last);

            printf("Got packet: %i\n", length);

            if (delegate != nullptr)
            {
                delegate->simpleDataPacketProtocolDelegateDidProcessPacket(newVec);
            }

            // Remove the data from buffer
            buffer.erase(buffer.begin(), buffer.begin() + sizeof(length) + length);

            // Read the next length
            // Had to add this check to fix an out of bounds access on Windows.
            if (buffer.size() > 0)
            {
                memcpy(&length, &buffer[0], sizeof length);
                length = ntohl(length);
            }
            else
            {
                length = 0;
            }
        }
    }

    return 0;
}
}
