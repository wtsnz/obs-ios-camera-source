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

#include "Channel.hpp"
#include <iostream>

namespace portal {

Channel::Channel(int port_, int conn_)
{
	port = port_;
	conn = conn_;

	setState(State::Disconnected);
}

Channel::~Channel()
{
	running = false;
	WaitForInternalThreadToExit();
	portal_log("%s: Deallocating\n", __func__);
}

bool Channel::start()
{
	if (getState() == State::Connected) {
		return false;
	}

	running = StartInternalThread();

	if (running == true) {
		setState(State::Connecting);
	}

	return running;
}

bool Channel::close()
{
	running = false;

	//// Wait for exit if close() wasn't called on the internal thread
	if (std::this_thread::get_id() != _thread.get_id()) {
		WaitForInternalThreadToExit();
	}

	auto ret = usbmuxd_disconnect(conn);

	return ret;
}

bool Channel::send(std::vector<char> data)
{
	if (getState() != State::Connected) {
		return false;
	}

	uint32_t sentBytes = 0;

	usbmuxd_send(conn, &data[0], data.size(), &sentBytes);
}

void Channel::setState(State state)
{
	if (state == getState()) {
		return;
	}

	//std::cout << "Channel:setState: " << state << std::endl;
	_state = state;

	if (auto delegate = this->delegate.lock()) {
		delegate->channelDidChangeState(state);
	}
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
	//if (_thread == nullptr) {
	//	return;
	//}
	std::unique_lock<std::mutex> lock(worker_mutex);

	if (_thread.joinable() == true) {
		_thread.join();
	}

	//_thread = nullptr;
	lock.unlock();
}

void Channel::StopInternalThread()
{
	running = false;
}

// TODO: https://stackoverflow.com/questions/58477291/function-exceeds-stack-size-consider-moving-some-data-to-heap-c6262
void Channel::InternalThreadEntry()
{
	while (running) {

		const uint32_t numberOfBytesToAskFor =
			65536; // (1 << 16); // This is the value in DarkLighting
		uint32_t numberOfBytesReceived = 0;

		char buffer[numberOfBytesToAskFor];

		int ret = usbmuxd_recv_timeout(conn, (char *)&buffer,
					       numberOfBytesToAskFor,
					       &numberOfBytesReceived, 10);

		if (ret == 0) {

			if (getState() == State::Connecting) {
				setState(State::Connected);
			}

			if (numberOfBytesReceived > 0) {
				if (running) {
					if (auto spt = delegate.lock()) {
						// copy the char* bytes into the vector
						auto vector =
							std::vector<char>();
						vector.insert(
							vector.end(), buffer,
							buffer +
								numberOfBytesReceived);
						spt->channelDidReceiveData(
							vector);
					}
				}
			}
		} else {
			portal_log("There was an error receiving data");
			close();
			//running = false;
			setState(State::Errored);
		}
	}
}

} // namespace portal
