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

#include "Device.hpp"
#include <list>
#include <sstream>
#include <vector>

namespace portal
{

    Device::DeviceMap Device::s_devices;
    //Device::ChannelsVec Device::s_connectedChannels;

    Device::Device(const usbmuxd_device_info_t &device) : _connected(false),
    _device(device),
    _uuid(_device.udid),
    _productId(std::to_string(_device.product_id))
    {
        s_devices[_uuid].push_back(this);
        portal_log("Added %p to device list\n", this);
    }

    Device::Device(const Device &other) : _connected(other._connected),
    _device(other._device),
    _uuid(other._uuid)
    {
        s_devices[_uuid].push_back(this);
        portal_log("Added %p to device list (copy)\n", this);
    }

    Device &Device::operator=(const Device &rhs)
    {
        removeFromDeviceList();

        this->_connected = rhs._connected;
        this->_device = rhs._device;
        this->_uuid = rhs._uuid;
        this->_productId = rhs._productId;

        s_devices[_uuid].push_back(this);

        return *this;
    }

    const std::string &Device::uuid() const
    {
        return _uuid;
    }

    int Device::usbmuxdHandle() const
    {
        return _device.handle;
    }

    uint16_t Device::productID() const
    {
        return _device.product_id;
    }

    int Device::connect(uint16_t port, std::shared_ptr<ChannelDelegate> newChannelDelegate, int connectTimeoutMs)
    {
        int retval = 0;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(connectTimeoutMs);

        while (std::chrono::steady_clock::now() < deadline) {
            int conn = usbmuxd_connect(_device.handle, port);

            if (conn > 0)
            {
                connectedChannel = std::shared_ptr<Channel>(new Channel(port, conn));
                connectedChannel->configureProtocolDelegate();
                connectedChannel->setDelegate(newChannelDelegate);
                return 0;
            }

            retval = conn;
        }

        return retval;
    }

    void Device::disconnect()
    {
        if (isConnected() == false) {
            return;
        }

        connectedChannel->close();
        connectedChannel = nullptr;
    }

    bool Device::isConnected() const
    {
        if (connectedChannel == nullptr) {
            return false;
        }

        return true;
    }

    void Device::removeFromDeviceList()
    {
        //Remove this device from the tracked devices
        std::vector<Device *> &devs = s_devices[_uuid];
        std::vector<Device *>::iterator it = std::find(devs.begin(), devs.end(), this);
        if (it != devs.end())
            devs.erase(it);
    }

    Device::~Device()
    {
        disconnect();
        removeFromDeviceList();
        portal_log("Removed %p from device list\n", this);
    }

    std::ostream &operator<<(std::ostream &os, const Device &v)
    {
        os << "v.productName()"
        << " [ " << v.uuid() << " ]";
        return os;
    }
}

