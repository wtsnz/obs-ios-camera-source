/*
 portal
 Copyright (C) 2018    Will Townsend <will@townsend.io>

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

        if (client->delegate != NULL) {
            client->delegate->portalDidUpdateDeviceList(client->_devices);
        }
    }

    Portal::Portal(PortalDelegate *delegate) : _listening(false)
    {
        this->delegate = delegate;

#if PORTAL_DEBUG_LOG_ENABLED
        libusbmuxd_set_debug_level(10);
#endif

        // Load the device list
        reloadDeviceList();

        startListeningForDevices();
    }

    void Portal::connectToDevice(Device::shared_ptr device)
    {
        // Disconnect to previous device
        if (_device) {
            portal_log("%s: Disconnecting from old device \n", __func__);
            _device->disconnect();
            _device = nullptr;
        }

        _device = device;

        portal_log("PORTAL (%p): Connecting to device: %s (%s)\n", this, device->getProductId().c_str(), device->uuid().c_str());

        // Connect to the device with the channel delegate.
        device->connect(2345, shared_from_this(), 1200);
    }

    void Portal::removeDisconnectedDevices()
    {
        // Find removed devices.
        std::list<Device::shared_ptr> devicesToRemove;

        std::for_each(_devices.begin(), _devices.end(),
                      [&](std::map<int, Device::shared_ptr>::value_type &deviceMap) {
                          if (deviceMap.second->isConnected() == false) {
                              devicesToRemove.push_back(deviceMap.second);
                          }
                      }
                      );

        // Remove the unplugged devices.
        std::for_each(devicesToRemove.begin(), devicesToRemove.end(), [this](Device::shared_ptr device) {
            this->removeDevice(device->_device);
        });
    }

    void Portal::addConnectedDevices()
    {
        // Add the currently connected devices
        int connectedDeviceCount = 0;
        usbmuxd_device_info_t *devicelist = NULL;
        connectedDeviceCount = usbmuxd_get_device_list(&devicelist);

        if (connectedDeviceCount > 0) {

            usbmuxd_device_info_t device_info;
            memset(&device_info, 0, sizeof(usbmuxd_device_info_t));

            for (int i = 0; i < connectedDeviceCount; i++) {
                device_info = devicelist[i];
                addDevice(device_info);
            }
        }
    }

    void Portal::reloadDeviceList()
    {
        removeDisconnectedDevices();
        addConnectedDevices();
    }

    // BUG: Listening for devices only works when there is one instance of the plugin
    int Portal::startListeningForDevices()
    {
        //Subscribe for device connections
        int status = usbmuxd_subscribe(pt_usbmuxd_cb, this);
        if (status)
        {
            return -1;
        }

        _listening = true;
        portal_log("%s: Listening for devices \n", __func__);

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
        // Filter out network connected devices
        if (strcmp(device.connection_type, "Network") == 0)
        {
            return;
        }

        if (_devices.find(device.handle) == _devices.end())
        {
            Device::shared_ptr sp = Device::shared_ptr(new Device(device));
            _devices.insert(DeviceMap::value_type(device.handle, sp));
            portal_log("PORTAL (%p): Added device: %i (%s)\n", this, device.product_id, device.udid);
        }
    }

    void Portal::removeDevice(const usbmuxd_device_info_t &device)
    {
        DeviceMap::iterator it = _devices.find(device.handle);

        if (it != _devices.end())
        {
            it->second->disconnect();
            _devices.erase(it);
            portal_log("PORTAL (%p): Removed device: %i (%s)\n", this, device.product_id, device.udid);
        }
    }

    void Portal::channelDidReceivePacket(std::vector<char> packet, int type, int tag)
    {
        if (delegate != NULL) {
            delegate->portalDeviceDidReceivePacket(packet, type, tag);
        }
    }

    void Portal::channelDidStop()
    {
        portal_log("Channel Did Stop in portal\n");
    }

    Portal::~Portal()
    {
        if (_listening)
        {
            usbmuxd_unsubscribe();
        }
    }
}

