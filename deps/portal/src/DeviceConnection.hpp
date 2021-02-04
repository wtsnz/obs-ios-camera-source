/*
 portal
 Copyright (C) 2020 Will Townsend <will@townsend.io>

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

#pragma once

#include <thread>
#include <usbmuxd.h>

#include "Device.hpp"
#include "Protocol.hpp"
#include "Channel.hpp"

#include "logging.h"

namespace portal {

class DeviceConnection : public Channel::Delegate, public std::enable_shared_from_this<DeviceConnection> {
public:

    enum class State {
	    // Not attempted to connect yet
        Disconnected = 0,
	// Connecting
        Connecting,
	// Connected & recieving data
        Connected,
	// Failed to connect
	FailedToConnect,
	// Was connected, but the connected errored out for some reason
        Errored
    };

    class Delegate {
    public:
        virtual void connectionDidChangeState(std::shared_ptr<DeviceConnection> deviceConnection, DeviceConnection::State state) = 0;
        virtual void connectionDidRecieveData(std::shared_ptr<DeviceConnection> deviceConnection, std::vector<char> data) = 0;
        virtual void connectionDidFail(std::shared_ptr<DeviceConnection> deviceConnection) = 0;
        virtual ~Delegate(){};
    };

    DeviceConnection(Device::shared_ptr device, int port);
    ~DeviceConnection();

    bool connect();
    bool disconnect();
    bool send(std::vector<char> data);

    std::shared_ptr<DeviceConnection> getptr()
    {
        return shared_from_this();
    }

    void setDelegate(std::shared_ptr<Delegate> newDelegate)
    {
        delegate = newDelegate;
    }

    int getPort()
    {
        return port;
    }

    State getState() {
	    return _state;
	}

    Device::shared_ptr getDevice() { return device; };

    void channelDidChangeState(Channel::State state);
    void channelDidReceiveData(std::vector<char> data);
    void channelDidReceivePacket(std::vector<char> packet, int type, int tag);
    void channelDidStop();

private:
    int port;
    Device::shared_ptr device;
    
    void setState(State state);

    State _state;

    //dispatch_queue queue;

    // The data channel
    std::shared_ptr<Channel> channel;
    std::weak_ptr<Delegate> delegate;
};

} // namespace portal
