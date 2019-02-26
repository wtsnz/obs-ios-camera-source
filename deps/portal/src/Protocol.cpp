/*
 portal
 Copyright (C) 2018    Will Townsend <will@townsend.io>

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
        if (dataLength > 0)
        {
            // Add data recieved to the end of buffer.
            buffer.insert(buffer.end(), data, data + dataLength);
        }

        uint32_t length = 0;
        // Ensure that the data inside the buffer is at least as big
        // as the length variable (32 bit int)
        // and then read it out
        if (buffer.size() < sizeof(PortalFrame))
        {
            return -1;
        }
        else
        {
            // Read the portal frame out
            PortalFrame frame;

            memcpy(&frame, &buffer[0], sizeof(PortalFrame));
            length = ntohl(length);

            frame.version = ntohl(frame.version);
            frame.type = ntohl(frame.type);
            frame.tag = ntohl(frame.tag);
            frame.payloadSize = ntohl(frame.payloadSize);

            if (frame.payloadSize == 0) {
                printf("Payload was 0");
                buffer.erase(buffer.begin(), buffer.begin() + sizeof(PortalFrame) + frame.payloadSize);
                return -1;
            }

            // Read payload size now..
            // Check if we've got all the data for the packet

            if (buffer.size() > (sizeof(PortalFrame) + frame.payloadSize)) {

                // Read length bytes as that is the packet
                std::vector<char>::const_iterator first = buffer.begin() + sizeof(PortalFrame);
                std::vector<char>::const_iterator last = buffer.begin() + sizeof(PortalFrame) + frame.payloadSize;
                std::vector<char> newVec(first, last);

                std::shared_ptr<SimpleDataPacketProtocolDelegate> strongDelegate = delegate.lock();
                if (strongDelegate) {
                    strongDelegate->simpleDataPacketProtocolDelegateDidProcessPacket(newVec, frame.type, frame.tag);
                }

                // Remove the data from buffer
                buffer.erase(buffer.begin(), buffer.begin() + sizeof(PortalFrame) + frame.payloadSize);

            } else {

                // We haven't got the data for the packet just yet, so wait for next time!
                return -1;
            }

        }

        return 0;
    }
}

