/*
 portal
 Copyright (C) 2018 Will Townsend <will@townsend.io>

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
#include <mutex>
#include <usbmuxd.h>

#include "Protocol.hpp"

#include "logging.h"

#include <iostream>
#include <type_traits>
template<typename T>
std::ostream &operator<<(
	typename std::enable_if<std::is_enum<T>::value,
	std::ostream>::type &stream,
	const T &e
)
{
	return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

namespace portal {

class Channel : public std::enable_shared_from_this<Channel> {

public:
	enum class State { Disconnected = 0, Connecting, Connected, Errored };

	class Delegate {
	public:
		virtual void channelDidReceiveData(std::vector<char> data) = 0;
		virtual void channelDidChangeState(Channel::State state) = 0;
		virtual void channelDidStop() = 0;
		virtual ~Delegate(){};
	};

	Channel(int port, int sfd);
	~Channel();

	bool start();
	bool close();

	bool send(std::vector<char> data);

	std::shared_ptr<Channel> getptr() { return shared_from_this(); }

	void setDelegate(std::shared_ptr<Delegate> newDelegate)
	{
		delegate = newDelegate;
	}

	int getPort() { return port; }

private:
	int port;
	int conn;

	void setState(State state);
	State getState() { return _state; };

	State _state;

	std::weak_ptr<Delegate> delegate;

    std::atomic_bool running = false;

	bool StartInternalThread();
	void WaitForInternalThreadToExit();
	void StopInternalThread();
	void InternalThreadEntry();

	static void *InternalThreadEntryFunc(void *This)
	{
		((portal::Channel *)This)->InternalThreadEntry();
		return NULL;
	}

	std::mutex worker_mutex;
	std::thread _thread;
};

} // namespace portal
