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

#include "Channel.hpp"

namespace portal
{

    Channel::Channel(int port_, int conn_)
    {
        port = port_;
        conn = conn_;

        protocol = std::make_unique<SimpleDataPacketProtocol>();

        running = StartInternalThread();
    }

    Channel::~Channel()
    {
        running = false;
        WaitForInternalThreadToExit();
        portal_log("%s: %d: Deallocating\n", __func__, conn);
    }

    void Channel::close()
    {
        running = false;
        WaitForInternalThreadToExit();
        usbmuxd_disconnect(conn);
    }

    /** Returns true if the thread was successfully started, false if there was an error starting the thread */
    bool Channel::StartInternalThread()
    {
        _thread = std::thread(InternalThreadEntryFunc, this);
        return true;
    }

    /** Will not return until the internal thread has exited. */
    void Channel::WaitForInternalThreadToExit()
    {
        if (_thread.joinable())
        {
            _thread.join();
        }
    }

    void Channel::StopInternalThread()
    {
        running = false;
    }

    void Channel::InternalThreadEntry()
    {
        while (running)
        {

            const uint32_t numberOfBytesToAskFor = 65536; // (1 << 16); // This is the value in DarkLighting
            uint32_t numberOfBytesReceived = 0;

            char buffer[numberOfBytesToAskFor];

            int ret = usbmuxd_recv_timeout(conn, (char *)&buffer, numberOfBytesToAskFor, &numberOfBytesReceived, 100);

            if (ret == 0)
            {
                if (numberOfBytesReceived > 0)
                {
                    if (running) {
                        protocol->processData((char *)buffer, numberOfBytesReceived);
                    }
                }
            }
            else
            {
                portal_log("%d: There was an error receiving data\n", conn);
                running = false;
            }
        }
    }

    void Channel::simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet, int type, int tag)
    {
        std::shared_ptr<ChannelDelegate> strongDelegate = delegate.lock();
        if (strongDelegate) {
            strongDelegate->channelDidReceivePacket(packet, type, tag);
        }
    }
}

