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

#include <vector>

namespace portal
{

class SimpleDataPacketProtocolDelegate
{
  public:
    virtual void simpleDataPacketProtocolDelegateDidProcessPacket(std::vector<char> packet) = 0;
    virtual ~SimpleDataPacketProtocolDelegate(){};
};

class SimpleDataPacketProtocol
{
  public:
    SimpleDataPacketProtocol();
    ~SimpleDataPacketProtocol();

    int processData(char *data, int dataLength);

    void reset();

    void setDelegate(SimpleDataPacketProtocolDelegate *newDelegate)
    {
        delegate = newDelegate;
    }

  private:
    SimpleDataPacketProtocolDelegate *delegate;

    std::vector<char> buffer;
};
}
