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

#include "DeviceManager.hpp"
#include <algorithm>
#include <set>

namespace portal {

/**
 usbmuxd callback
*/
void pt_device_manager_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data)
{
	DeviceManager *client = static_cast<DeviceManager *>(user_data);

	switch (event->event) {
	case UE_DEVICE_ADD:
		client->addDevice(event->device);
		break;
	case UE_DEVICE_REMOVE:
		client->removeDevice(event->device);
		break;
	}

	if (client->onDeviceManagerDidUpdateDeviceListCallback) {
		client->onDeviceManagerDidUpdateDeviceListCallback(
			client->getDevices());
	}
}

DeviceManager::DeviceManager()
{
	_state = State::Disconnected;
}

DeviceManager::~DeviceManager()
{
	worker_stopping = true;
	worker_condition.notify_all();

	if (worker_thread.joinable()) {
		worker_thread.join();
	}
}

void DeviceManager::worker_loop()
{
	while (!worker_stopping) {
		std::unique_lock<std::mutex> lock(worker_mutex);

		updateDevices();

		worker_condition.wait_for(lock, std::chrono::seconds(1));
		lock.unlock();
	}
}

void DeviceManager::updateDevices()
{
	auto changed_devices = false;

	usbmuxd_device_info_t *device_list = NULL;
	int count = usbmuxd_get_device_list(&device_list);
	usbmuxd_device_info_t device_info;
	memset(&device_info, 0, sizeof(usbmuxd_device_info_t));

	std::set<std::string> connected_uuids;

	// Add any new devices
	for (int i = 0; i < count; i++) {
		device_info = device_list[i];

		

		// The addDevice method will tell us if the device was added
		auto did_add_device = addDevice(device_info);
		connected_uuids.insert(std::string(device_info.udid));

		if (did_add_device) {
			changed_devices = true;
		}
	}

	// Remove any devices that are no longer connected
	auto devices_copy = devices;
	std::for_each(devices_copy.begin(), devices_copy.end(),
		      [this, connected_uuids, &changed_devices](auto device) {
			      auto uuid = device.first;

			      if (connected_uuids.find(uuid) == connected_uuids.end()) {
				      removeDevice(device.second->getUsbmuxdInfo());
				      changed_devices = true;
			      }
		      });


	// Notify callbacks if we've made changes to the devices
	if (changed_devices && onDeviceManagerDidUpdateDeviceListCallback) {
		onDeviceManagerDidUpdateDeviceListCallback(getDevices());
	}
}

bool DeviceManager::start()
{
	if (getState() != State::Disconnected) {
		return false;
	}

	updateDevices();

	setState(State::Connected);

	worker_thread = std::thread(&DeviceManager::worker_loop, this);

	return true;
}

bool DeviceManager::stop()
{
	if (getState() != State::Connected) {
		return false;
	}

	//Always returns 0
	//usbmuxd_unsubscribe();

	setState(State::Disconnected);

	return true;
}

bool DeviceManager::addDevice(const usbmuxd_device_info_t &device)
{
	// Filter out network connected devices
	if (device.conn_type == CONNECTION_TYPE_NETWORK) {
		return false;
	}

	DeviceMap::iterator it = devices.find(device.udid);

        // Device does exist, update the usbmuxd handle
	if (it != devices.end()) {
		auto devicePtr = it->second;
		devicePtr->setUsbmuxDevice(device);
	}

	// Device doesn't exist, so add it
	if (it == devices.end()) {
		Device::shared_ptr sp = Device::shared_ptr(new Device(device));
		devices.insert(DeviceMap::value_type(device.udid, sp));
		portal_log("PORTAL (%p): Added device: %i (%s)\n", this,
			   device.product_id, device.udid);

		if (onDeviceManagerDidAddDeviceCallback) {
			onDeviceManagerDidAddDeviceCallback(sp);
		}

		return true; // We did add the device
	}


	return false;
}

void DeviceManager::removeDevice(const usbmuxd_device_info_t &device)
{
	DeviceMap::iterator it = devices.find(device.udid);
	auto sp = it->second;

	if (it != devices.end()) {
		devices.erase(it);
		portal_log("PORTAL (%p): Removed device: %i (%s)\n", this,
			   device.product_id, device.udid);
	}

	if (onDeviceManagerDidRemoveDeviceCallback) {
		onDeviceManagerDidRemoveDeviceCallback(sp);
	}
}

void DeviceManager::setState(State state)
{
	if (getState() == state) {
		return;
	}

	_state = state;

	if (onDeviceManagerDidChangeStateCallback) {
		onDeviceManagerDidChangeStateCallback(state);
	}
}

} // namespace portal
