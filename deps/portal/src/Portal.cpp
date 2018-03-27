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

#include <iostream>

#include "Portal.hpp"

namespace portal
{
/**
usbmuxd callback
*/
void pt_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data)
{
	Portal *client = static_cast<Portal *>(user_data);

	switch (event->event)
	{
	case UE_DEVICE_ADD:
		client->addDevice(event->device);
		break;
	case UE_DEVICE_REMOVE:
		client->removeDevice(event->device);
		break;
	}
}

Portal::Portal() : _listening(false)
{
}

int Portal::startListeningForDevices()
{
	//Subscribe for device connections
	int status = usbmuxd_subscribe(pt_usbmuxd_cb, this);
	if (status)
	{
		return -1;
	}

	_listening = true;
	printf("%s: Listening for devices \n", __func__);

	return 0;
}

void Portal::stopListeningForDevices()
{
	if (_listening)
	{
		//Always returns 0
		usbmuxd_unsubscribe();
		_listening = false;
	}
}

bool Portal::isListening()
{
	return _listening;
}

void Portal::addDevice(const usbmuxd_device_info_t &device)
{
	if (_devices.find(device.handle) == _devices.end())
	{

		Device::shared_ptr sp = Device::shared_ptr(new Device(device));
		_devices.insert(DeviceMap::value_type(device.handle, sp));

		// Connect to the device
		// This port is "the" port.
		sp->connect(2345, this);
	}
}

void Portal::removeDevice(const usbmuxd_device_info_t &device)
{
	DeviceMap::iterator it = _devices.find(device.handle);

	if (it != _devices.end())
	{
		_devices.erase(it);
	}
}

void Portal::channelDidReceivePacket(std::vector<char> packet)
{
	printf("Portal chanel recive\n");
	if (delegate != nullptr)
	{
		delegate->portalDeviceDidReceivePacket(packet);
	}
}

void Portal::channelDidStop()
{
	std::cout << "Channel Did Stop in portal\n";
}

Portal::~Portal()
{
	if (_listening)
	{
		usbmuxd_unsubscribe();
	}
}
}
