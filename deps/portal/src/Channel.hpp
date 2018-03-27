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

#include <usbmuxd.h>
#include <thread>

#include "Protocol.hpp"

namespace portal
{

class ChannelDelegate
{
  public:
    virtual void channelDidReceivePacket(std::vector<char> packet) = 0;
    virtual void channelDidStop() = 0;
    virtual ~ChannelDelegate(){};
};

class Channel : public SimpleDataPacketProtocolDelegate
{
  public:
    Channel(int port, int sfd);
    ~Channel();

    void simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet);

    void setDelegate(ChannelDelegate *newDelegate)
    {
        delegate = newDelegate;
    }

  private:
    int port;
    int conn;

    void setPacketDelegate(SimpleDataPacketProtocolDelegate *newDelegate)
    {
        protocol.setDelegate(newDelegate);
    }

    SimpleDataPacketProtocol protocol;

    ChannelDelegate *delegate;

    bool running = false;

    bool StartInternalThread();
    void WaitForInternalThreadToExit();
    void StopInternalThread();
    void InternalThreadEntry();

    static void *InternalThreadEntryFunc(void *This)
    {
        ((portal::Channel *)This)->InternalThreadEntry();
        return NULL;
    }

    std::thread _thread;
};
}
