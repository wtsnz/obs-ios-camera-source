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

	return usbmuxd_send(conn, &data[0], data.size(), &sentBytes);
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
    running = false;
    std::unique_lock<std::mutex> lock(worker_mutex);
    if (_thread.joinable()) {
        _thread.join();
    }
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
        std::unique_lock<std::mutex> lock(worker_mutex);

        if (getState() == State::Errored) {
            return;
        }

        // Receive some data
        const uint32_t numberOfBytesToAskFor = 1 << 18; // 262,144
        uint32_t numberOfBytesReceived = 0;
        auto vector = std::vector<char>(numberOfBytesToAskFor);

        int ret = usbmuxd_recv_timeout(conn, vector.data(),
                           numberOfBytesToAskFor,
                           &numberOfBytesReceived, 1000);

		if (ret == 0) {
			if (getState() == State::Connecting) {
				setState(State::Connected);
			}

			if (numberOfBytesReceived > 0) {
				vector.resize(numberOfBytesReceived);

                if (auto spt = delegate.lock()) {
                    spt->channelDidReceiveData(
                        vector);
                }
			}
            lock.unlock();
        } else if (ret == -ETIMEDOUT) {

            // Timed out waiting for data.
            std::cout << "Timed out" << std::endl;
            lock.unlock();

		} else {
            // -ECONNRESET
            // Unlock now as the `close()` function also requires a lock
            lock.unlock();
			portal_log("There was an error receiving data");
			close();
			setState(State::Errored);
		}
	}
}

} // namespace portal
