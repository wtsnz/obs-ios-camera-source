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

#include <usbmuxd.h>
#include <vector>
#include <map>
#include <algorithm>
#include <list>

#include "logging.h"
#include "Device.hpp"

typedef void (*portal_channel_receive_cb_t)(char *buffer, int buffer_len, void *user_data);

namespace portal
{

    class PortalDelegate
    {
    public:
        virtual void portalDeviceDidReceivePacket(std::vector<char> packet, int type, int tag) = 0;
        virtual void portalDidUpdateDeviceList(std::map<int, Device::shared_ptr>) = 0;
        virtual ~PortalDelegate(){};
    };

    class Portal : public ChannelDelegate, public std::enable_shared_from_this<Portal>
    {
    public:
        typedef std::map<int, Device::shared_ptr> DeviceMap;

        Portal(PortalDelegate *delegate);
        ~Portal();

        std::shared_ptr<Portal> getptr()
        {
            return shared_from_this();
        }

        int startListeningForDevices();
        void stopListeningForDevices();
        bool isListening();

        void connectToDevice(Device::shared_ptr device);

        void reloadDeviceList();

        Portal::DeviceMap getDevices() {
            return _devices;
        }

        PortalDelegate *delegate;

        Device::shared_ptr _device;
    private:

        bool _listening;
        Portal::DeviceMap _devices;

        Portal(const Portal &other);
        Portal &operator=(const Portal &other);

        void removeDisconnectedDevices();
        void addConnectedDevices();

        void addDevice(const usbmuxd_device_info_t &device);
        void removeDevice(const usbmuxd_device_info_t &device);

        friend void pt_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data);

        void channelDidReceivePacket(std::vector<char> packet, int type, int tag);
        void channelDidStop();
    };

}

