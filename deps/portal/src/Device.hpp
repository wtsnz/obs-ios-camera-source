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

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <usbmuxd.h>

#include "logging.h"
#include "Protocol.hpp"
#include "Channel.hpp"
#include <optional>

namespace portal
{

    class Portal;

    class DeviceDelegate
    {
    public:
        virtual void deviceDidConnect() = 0;
        virtual void deviceDidDisconnect() = 0;

        virtual ~DeviceDelegate(){};
    };

    /**
     Represents an iOS device detected by usbmuxd. This class
     can be used to retrieve info and initiate TCP sessions
     with the iOS device.
     */
    class Device : public std::enable_shared_from_this<Device>
    {
    public:
        std::shared_ptr<Device> getptr()
        {
            return shared_from_this();
        }

        /**
         Default constructor for the class.
         *
         @param device The underlying usbmuxd device.
         */
        explicit Device(const usbmuxd_device_info_t &device);
        Device(const Device &other);
        Device &operator=(const Device &rhs);

        /**
         Returns whether or not the device is connected
         *
         @return true if connected, otherwise false.
         */
        bool isConnected() const;

        void disconnect();

        /**
         Gets the usbmuxd handle for the device. This handle can be
         used to comminucate directly with the device using libusbmuxd.
         *
         @return The usbmuxd handle
         */
        int usbmuxdHandle() const;

        /**
         Returns the 40 character UUID associated with the device.
         *
         @return The devices UUID.
         */
        const std::string &uuid() const;

        /**
         Returns the name of the product.
         *
         @return The print friendly version of the product name.
         */
        // const std::string & productName() const;

        /**
         Returns the product ID. This is used to determine the product name.
         *
         @return The 16 bit product ID.
         */
        uint16_t productID() const;

        int connect(uint16_t port, std::shared_ptr<Channel::Delegate> channelDelegate, int attempts);

        ~Device();

        typedef std::map<std::string, std::vector<Device *>> DeviceMap;
        typedef std::vector<std::shared_ptr<Channel>> ChannelsVec;
        typedef std::shared_ptr<Device> shared_ptr;

        void setDelegate(DeviceDelegate *newDelegate)
        {
            delegate = newDelegate;
        }

        std::string getProductId() {
            return _productId;
        }

        std::optional<std::string> name;

	void setUsbmuxDevice(const usbmuxd_device_info_t device);
	usbmuxd_device_info_t getUsbmuxdInfo() { return _device; }

    private:
        DeviceDelegate *delegate = nullptr;

        std::shared_ptr<Channel> connectedChannel;

        bool _connected;
        usbmuxd_device_info_t _device;
        std::string _uuid;
        std::string _productId;

        friend class Portal;
        friend std::ostream &operator<<(std::ostream &os, const Device &v);
    };
}

