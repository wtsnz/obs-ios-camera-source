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

#include <thread>
#include <condition_variable>
#include <functional>

#include <usbmuxd.h>

#include "Device.hpp"
#include "Protocol.hpp"

#include "logging.h"

namespace portal {

class DeviceManager : public std::enable_shared_from_this<DeviceManager> {
public:

    enum class State {
        Disconnected = 0,
        Connecting,
        Connected
    };

    class Delegate {
    public:
        virtual void deviceManagerDidUpdateDeviceList(std::map<std::string, Device::shared_ptr> devices) = 0;
        virtual void deviceManagerDidAddDevice(Device::shared_ptr device) = 0;
        virtual void deviceManagerDidRemoveDevice(Device::shared_ptr device) = 0;
        virtual void deviceManagerDidChangeState(DeviceManager::State state) = 0;
        virtual ~Delegate(){};
    };

    std::function<void(std::map<std::string, Device::shared_ptr> devices)>
	    onDeviceManagerDidUpdateDeviceListCallback;
    std::function<void(Device::shared_ptr device)> onDeviceManagerDidAddDeviceCallback;
    std::function<void(Device::shared_ptr device)>
	    onDeviceManagerDidRemoveDeviceCallback;
    std::function<void(DeviceManager::State state)>
	    onDeviceManagerDidChangeStateCallback;

    typedef std::map<std::string, Device::shared_ptr> DeviceMap;

    DeviceManager();
    ~DeviceManager();

    bool start();
    bool stop();

    void updateDevices();

    std::shared_ptr<DeviceManager> getptr()
    {
        return shared_from_this();
    }

    DeviceManager::DeviceMap getDevices() {
        return devices;
    }

    State getState() { return _state; };
    void setState(State state);

    bool addDevice(const usbmuxd_device_info_t &device);
    void removeDevice(const usbmuxd_device_info_t &device);


    private:
    int port;
    State _state;

    DeviceManager::DeviceMap devices;

    // Devices Worker state
    std::atomic_bool worker_stopping = false;
    std::condition_variable worker_condition;
    std::mutex worker_mutex;
    std::thread worker_thread;
    void worker_loop();

    friend void pt_usbmuxd_cb(const usbmuxd_event_t* event, void* user_data);
};

} // namespace portal
