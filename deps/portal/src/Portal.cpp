#include "Portal.hpp"
#include <iostream>


namespace portal
{
/**
usbmuxd callback
*/
void pt_usbmuxd_cb(const usbmuxd_event_t *event, void *user_data)
{
	Portal *client = static_cast<Portal*>(user_data);

	switch(event->event)
	{
		case UE_DEVICE_ADD:
			client->addDevice(event->device);
		break;
		case UE_DEVICE_REMOVE:
			client->removeDevice(event->device);
		break;
	}
}

Portal::Portal() :
	_listening(false)
	// _devices()
{
}

int Portal::startListeningForDevices()
{
	//Subscribe for device connections
	int status = usbmuxd_subscribe(pt_usbmuxd_cb, this);
	if(status)
	{
		return -1;
	}
	
	_listening = true;
    printf("%s: Listening for devices \n", __func__);

	return 0;
}

void Portal::stopListeningForDevices()
{
	if(_listening)
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

void Portal::addDevice(const usbmuxd_device_info_t& device)
{
	if (_devices.find(device.handle) == _devices.end()) {

//        Device d = Device(device);
        Device::shared_ptr sp = Device::shared_ptr(new Device(device));
        _devices.insert(DeviceMap::value_type(device.handle, sp));
//        d.setDelegate(this);
        
        // Connect to the device
        sp->connect(2345, this);
    }
}

void Portal::removeDevice(const usbmuxd_device_info_t& device)
{
	DeviceMap::iterator it = _devices.find(device.handle);

	if(it != _devices.end())
	{
		_devices.erase(it);
	}
}

void Portal::channelDidReceivePacket(std::vector<char> packet)
{
    printf("Portal chanel recive\n");
    if (delegate != nullptr) {
        delegate->portalDeviceDidReceivePacket(packet);
    }
}
    
void Portal::channelDidStop()
{
    std::cout << "Channel Did Stop in portal\n";
}
    
Portal::~Portal()
{
	if(_listening)
	{
		usbmuxd_unsubscribe();
	}
}

}
