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

#ifndef PORTAL_SIMPLE_DATA_PACKET_PROTOCOL_H
#define PORTAL_SIMPLE_DATA_PACKET_PROTOCOL_H

#include <vector>

#include "logging.h"

namespace portal
{

    // This is what we send as the header for each frame.
    typedef struct _PortalFrame {
        // The version of the frame and protocol.
        uint32_t version;

        // Type of frame
        uint32_t type;

        // Unless zero, a tag is retained in frames that are responses to previous
        // frames. Applications can use this to build transactions or request-response
        // logic.
        uint32_t tag;

        // If payloadSize is larger than zero, *payloadSize* number of bytes are
        // following, constituting application-specific data.
        uint32_t payloadSize;

    } PortalFrame;

    class SimpleDataPacketProtocolDelegate
    {
    public:
        virtual void simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet, int type, int tag) = 0;
        virtual ~SimpleDataPacketProtocolDelegate(){};
    };

    class SimpleDataPacketProtocol: public std::enable_shared_from_this<SimpleDataPacketProtocol>
    {
    public:
        SimpleDataPacketProtocol();
        ~SimpleDataPacketProtocol();

        std::shared_ptr<SimpleDataPacketProtocol> getptr()
        {
            return shared_from_this();
        }

        int processData(char *data, int dataLength);

        void reset();

        void setDelegate(std::shared_ptr<SimpleDataPacketProtocolDelegate> newDelegate)
        {
            delegate = newDelegate;
        }

    private:
        std::weak_ptr<SimpleDataPacketProtocolDelegate> delegate;

        std::vector<char> buffer;
    };
}

#endif

