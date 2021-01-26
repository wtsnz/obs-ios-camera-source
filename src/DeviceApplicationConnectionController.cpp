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

#include <iostream>

#include "DeviceApplicationConnectionController.hpp"

DeviceApplicationConnectionController::DeviceApplicationConnectionController(
	std::shared_ptr<portal::DeviceConnection> deviceConnection)
{
	this->protocol = std::make_unique<portal::SimpleDataPacketProtocol>();
	this->deviceConnection = deviceConnection;
	this->device = deviceConnection->getDevice();
	should_reconnect = true;
	worker_stopping = false;
}

DeviceApplicationConnectionController::~DeviceApplicationConnectionController()
{
	should_reconnect = false;
	std::cout
		<< "DeviceApplicationConnectionController::~DeviceApplicationConnectionController()"
		<< std::endl;

	worker_stopping = true;
	worker_condition.notify_all();

	if (worker_thread.joinable()) {
		worker_thread.join();
		worker_thread_active = false;
	}
}

void DeviceApplicationConnectionController::start()
{
	should_reconnect = true;
	deviceConnection->setDelegate(shared_from_this());

	// only start the worker thread if it's not already started
	if (worker_thread_active == false) {
		worker_thread = std::thread(
			&DeviceApplicationConnectionController::worker_loop, this);
		worker_thread_active = true;
	}
	else {
		worker_condition.notify_all();
	}
}

void DeviceApplicationConnectionController::connect()
{
	should_reconnect = true;
	deviceConnection->setDelegate(shared_from_this());
	worker_condition.notify_all();
}

void DeviceApplicationConnectionController::disconnect()
{
	should_reconnect = false;
	deviceConnection->disconnect();
	protocol->reset();
}

void DeviceApplicationConnectionController::processPacket(
	portal::SimpleDataPacketProtocol::DataPacket packet)
{
	if (onProcessPacketCallback) {
		onProcessPacketCallback(packet);
	}
}

void DeviceApplicationConnectionController::worker_loop()
{
	while (!worker_stopping) {
		std::unique_lock<std::mutex> lock(worker_mutex);

		auto state = deviceConnection->getState();

		switch (state) {
		case portal::DeviceConnection::State::FailedToConnect:
		case portal::DeviceConnection::State::Errored:

			if (should_reconnect) {
				blog(LOG_DEBUG, "[obs-ios-camera-plugin] Device connection errored: reconnecting");
				this->deviceConnection->connect();
			}

			break;
			
		case portal::DeviceConnection::State::Disconnected:
			if (should_reconnect) {
				blog(LOG_DEBUG,
				     "[obs-ios-camera-plugin] Device connection disconnected: reconnecting if possible");
				this->deviceConnection->connect();
			}
			
			break;
		case portal::DeviceConnection::State::Connected:
			blog(LOG_DEBUG,
			     "[obs-ios-camera-plugin] Device connection is already connected. Doing nothing.");
			break;
        case portal::DeviceConnection::State::Connecting:
            blog(LOG_DEBUG,
                 "[obs-ios-camera-plugin] Device connection is already connecting. Doing nothing.");
                break;
        }

		worker_condition.wait_for(lock, std::chrono::seconds(1));
		lock.unlock();
	}

	worker_thread_active = false;
}

// Device Connection Delegate

void DeviceApplicationConnectionController::connectionDidChangeState(
	std::shared_ptr<portal::DeviceConnection> deviceConnection,
	portal::DeviceConnection::State state)
{
    UNUSED_PARAMETER(deviceConnection);
    UNUSED_PARAMETER(state);
}

void DeviceApplicationConnectionController::connectionDidRecieveData(
	std::shared_ptr<portal::DeviceConnection> deviceConnection,
	std::vector<char> data)
{
    UNUSED_PARAMETER(deviceConnection);

	auto packets = protocol->processData(data);
	std::for_each(packets.begin(), packets.end(),
		      [this](auto packet) { this->processPacket(packet); });
}

void DeviceApplicationConnectionController::connectionDidFail(
	std::shared_ptr<portal::DeviceConnection> deviceConnection)
{
    UNUSED_PARAMETER(deviceConnection);
}
