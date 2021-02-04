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

#include "DeviceConnection.hpp"

namespace portal {

#include <iostream>
#include <type_traits>
template <typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

DeviceConnection::DeviceConnection(Device::shared_ptr device, int port)
{
    this->port = port;
    this->device = device;
    this->_state = State::Disconnected;
}

DeviceConnection::~DeviceConnection()
{
    portal_log("%s: Deallocating\n", __func__);
}

bool DeviceConnection::connect()
{
	auto state = getState();

	if (state == State::Connecting) {
		return false;
	}

	setState(State::Connecting);

	auto connectTimeoutMs = 200;
	int retval = 0;
	auto deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(connectTimeoutMs);

	while (std::chrono::steady_clock::now() < deadline) {
		int socketHandle =
			usbmuxd_connect(device->usbmuxdHandle(), port);
		if (socketHandle > 0) {
			std::cout << "got connection: " << socketHandle
				  << std::endl;
			channel = std::make_shared<Channel>(port, socketHandle);
			channel->setDelegate(shared_from_this());
			channel->start();
			return 0;
		}
		retval = socketHandle;
	}

	setState(State::FailedToConnect);

	return true;
}

bool DeviceConnection::disconnect() 
{
    if (getState() != State::Connected) {
        return false;
    }

    auto ret = channel->close();
    if (ret == 0) {
        channel = nullptr; // Dealloc the channel
    }

    setState(State::Disconnected);

    return ret;
}

bool DeviceConnection::send(std::vector<char> data)
{
    return channel->send(data);
}

void DeviceConnection::channelDidChangeState(Channel::State state)
{
    if (state == Channel::State::Disconnected) {
        setState(State::Disconnected);
	    //channel->close();
	    //channel = std::shared_ptr<Channel>(new Channel(0, 0));
    }

    else if (state == Channel::State::Errored) {
	    setState(State::Errored);
	    //channel->close();
	    //channel = std::shared_ptr<Channel>(new Channel(0, 0));
    }

    else if (getState() == State::Connecting && state == Channel::State::Connected) {
        setState(State::Connected);
    }
}

void DeviceConnection::channelDidReceiveData(std::vector<char> data) 
{
    if (auto spt = delegate.lock()) {
        spt->connectionDidRecieveData(shared_from_this(), data);
    }
}


void DeviceConnection::channelDidReceivePacket(std::vector<char> packet, int type, int tag)
{

}

void DeviceConnection::channelDidStop()
{

}

void DeviceConnection::setState(State state)
{
    if (getState() == state) {
        return;
    }

    std::cout << "DeviceConnection::setState: " << state << std::endl;

    _state = state;

    if (auto spt = delegate.lock()) {
        spt->connectionDidChangeState(shared_from_this(), state);
    }
}

} // namespace portal
