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

#include "FFMpegVideoDecoder.h"

#include <algorithm>
#include <functional>

#include "Protocol.hpp"
#include "DeviceConnection.hpp"

class DeviceApplicationConnectionController
	: public portal::DeviceConnection::Delegate,
	  public std::enable_shared_from_this<
		  DeviceApplicationConnectionController> {
public:

	DeviceApplicationConnectionController(
		std::shared_ptr<portal::DeviceConnection> deviceConnection);
	~DeviceApplicationConnectionController();

	void start();
	void connect();
	void disconnect();

	std::function<void(portal::SimpleDataPacketProtocol::DataPacket packet)>
		onProcessPacketCallback;

	auto getState() { return deviceConnection->getState(); }

	std::string getDeviceUUID() { return device->uuid(); };

private:

	bool should_reconnect;

	// Worker state
	std::atomic_bool worker_stopping = false;
	std::atomic_bool worker_thread_active = false;
	std::condition_variable worker_condition;
	std::mutex worker_mutex;
	std::thread worker_thread;
	void worker_loop();

	std::unique_ptr<portal::SimpleDataPacketProtocol> protocol;
	std::shared_ptr<portal::Device> device;
	std::shared_ptr<portal::DeviceConnection> deviceConnection;

	void processPacket(portal::SimpleDataPacketProtocol::DataPacket packet);

	// Device Connection Delegate
	void connectionDidChangeState(
		std::shared_ptr<portal::DeviceConnection> deviceConnection,
		portal::DeviceConnection::State state);
	void connectionDidRecieveData(
		std::shared_ptr<portal::DeviceConnection> deviceConnection,
		std::vector<char> data);
	void connectionDidFail(
		std::shared_ptr<portal::DeviceConnection> deviceConnection);

};
