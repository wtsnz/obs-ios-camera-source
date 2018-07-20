/*
obs-ios-camera-source
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

#include <obs-module.h>
#include <chrono>
#include <Portal.hpp>
#include <usbmuxd.h>
#include <obs-avc.h>

#include "FFMpegVideoDecoder.h"
#include "FFMpegAudioDecoder.h"
//#include "VideoToolboxVideoDecoder.h"

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")

#define SETTING_DEVICE_UUID "setting_device_uuid"

class IOSCameraInput: public portal::PortalDelegate
{
  public:
	obs_source_t *source;
    obs_data_t *settings;
    
	portal::Portal portal;
	bool active = false;
	obs_source_frame frame;
    
    std::string deviceUUID;
    
//    VideoToolboxDecoder decoder;
    FFMpegVideoDecoder videoDecoder;
    FFMpegAudioDecoder audioDecoder;
    
	inline IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
        : source(source_), settings(settings), portal(this)
	{
        
        blog(LOG_INFO, "Creating instance of plugin!");
        
		memset(&frame, 0, sizeof(frame));
        
        loadSettings(settings);
        
		active = true;
        
        videoDecoder.source = source;
        videoDecoder.Init();
        
        audioDecoder.source = source;
        audioDecoder.Init();
        
        obs_source_set_async_unbuffered(source, true);
	}

	inline ~IOSCameraInput()
	{
        
	}

    void activate() {
        blog(LOG_INFO, "Activating");
        active = true;
    }
    
    void deactivate() {
        blog(LOG_INFO, "Deactivating");
        active = false;
    }
    
    void loadSettings(obs_data_t *settings) {
        auto device_uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);

        blog(LOG_INFO, "Loaded Settings: Connecting to device");
        
        connectToDevice(device_uuid);
    }
    
    void reconnectToDevice()
    {
        if (deviceUUID.size() < 1) {
            return;
        }
        
        connectToDevice(deviceUUID);
    }
    
    void connectToDevice(std::string uuid) {
        
        deviceUUID = std::string(uuid);
        
        blog(LOG_INFO, "Connecting to device");
        
        if (portal._device) {
            portal._device->disconnect();
            portal._device = nullptr;
        }
        
        // Find device
        auto devices = portal.getDevices();
        
        int index = 0;
        std::for_each(devices.begin(), devices.end(), [this, uuid, &index](std::map<int, portal::Device::shared_ptr>::value_type &deviceMap)
                      {
                          // Add the device name to the list
                          auto _uuid = deviceMap.second->uuid();
                          
//                          uuid.
//                          printf("comparing \n%s\n%s\n", _uuid.c_str(), uuid.c_str());
                          if (_uuid.compare(uuid) == 0) {
                              printf("comparing \n%s\n%s\n", _uuid.c_str(), uuid.c_str());
                              portal.connectToDevice(deviceMap.second);
                          }
                          
                          index++;
                      }
                      );
        
    }
    
	void portalDeviceDidReceivePacket(std::vector<char> packet, int type, int tag)
	{
        try
        {
            switch (type) {
                
                case 101: // Video Packet
                    this->videoDecoder.Input(packet, type, tag);
                    break;
                case 102: // Audio Packet
                    this->audioDecoder.Input(packet, type, tag);
                default:
                    break;
            }
        }
        catch (...)
        {
            // This isn't great, but I haven't been able to figure out what is causing
            // the exception that happens when
            //   the phone is plugged in with the app open
            //   OBS Studio is launched with the iOS Camera plugin ready
            // This also doesn't happen _all_ the time. Which makes this 'fun'..
            blog(LOG_INFO, "Exception caught...");
        }
	}
    
    void portalDidUpdateDeviceList(std::map<int, portal::Device::shared_ptr> deviceList)
    {
        // Update OBS Settings
        blog(LOG_INFO, "Updated device list");
        
        // If there is one device in the list, then we should attempt to connect to it.
        // This allows the majority use case (single device) to plugin in a device and it 'just work'.
        // Multiple devices will need to be configured.
        if (deviceList.size() == 1) {
            
            for (const auto& [index, device] : deviceList) {
                auto uuid = device.get()->uuid();
                
                // Set the setting so that the UI in OBS Studio is updated
                obs_data_set_string(this->settings, SETTING_DEVICE_UUID, uuid.c_str());
                
                // Connect to the device
                connectToDevice(uuid);
            }
            
        } else {
            reconnectToDevice();
        }
        
    }
    
};

#pragma mark - Settings Config

static bool refresh_devices(obs_properties_t *props, obs_property_t *p, void *data)
{
    UNUSED_PARAMETER(p);
    
    auto cameraInput =  reinterpret_cast<IOSCameraInput*>(data);
    
    cameraInput->portal.reloadDeviceList();
    auto devices = cameraInput->portal.getDevices();
    
    obs_property_t *dev_list = obs_properties_get(props, SETTING_DEVICE_UUID);
    obs_property_list_clear(dev_list);
    
    obs_property_list_add_string(dev_list, "None", "null");
    
    int index = 1;
    std::for_each(devices.begin(), devices.end(), [dev_list, &index](std::map<int, portal::Device::shared_ptr>::value_type &deviceMap)
                  {
                      // Add the device uuid to the list.
                      // It would be neat to grab the device name somehow, but that will likely require
                      // libmobiledevice instead of usbmuxd. Something to look into.
                      auto uuid = deviceMap.second->uuid().c_str();
                      obs_property_list_add_string(dev_list, uuid, uuid);
                      
                      // Disable the row if the device is selected as we can only
                      // connect to one device to one source.
                      // Disabled for now as I'm not sure how to sync status across
                      // multiple instances of the plugin.
//                      auto isConnected = deviceMap.second->isConnected();
//                      obs_property_list_item_disable(dev_list, index, isConnected);
                      
                      index++;
                  }
    );
    
    return true;
}

static bool reconnect_to_device(obs_properties_t *props, obs_property_t *p, void *data)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(p);
    
    auto cameraInput =  reinterpret_cast<IOSCameraInput* >(data);
    cameraInput->reconnectToDevice();
    
    return false;
}

#pragma mark - Plugin Callbacks

static const char *GetIOSCameraInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void *CreateIOSCameraInput(obs_data_t *settings, obs_source_t *source)
{
	IOSCameraInput *cameraInput = nullptr;

	try
	{
		cameraInput = new IOSCameraInput(source, settings);
	}
	catch (const char *error)
	{
		blog(LOG_ERROR, "Could not create device '%s': %s", obs_source_get_name(source), error);
	}

	return cameraInput;
}

static void DestroyIOSCameraInput(void *data)
{
	delete reinterpret_cast<IOSCameraInput *>(data);
}

static void DeactivateIOSCameraInput(void *data)
{
    auto cameraInput =  reinterpret_cast<IOSCameraInput*>(data);
    cameraInput->deactivate();
}

static void ActivateIOSCameraInput(void *data)
{
    auto cameraInput = reinterpret_cast<IOSCameraInput*>(data);
    cameraInput->activate();
}

static obs_properties_t *GetIOSCameraProperties(void *data)
{
    UNUSED_PARAMETER(data);
    obs_properties_t *ppts = obs_properties_create();
    
    obs_property_t *dev_list = obs_properties_add_list(ppts, SETTING_DEVICE_UUID,
                                                       "iOS Device",
                                                       OBS_COMBO_TYPE_LIST,
                                                       OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(dev_list, "", "");
    
    refresh_devices(ppts, dev_list, data);
    
    obs_properties_add_button(ppts, "setting_refresh_devices", "Refresh Devices", refresh_devices);
    obs_properties_add_button(ppts, "setting_button_connect_to_device", "Reconnect to Device", reconnect_to_device);
    
    return ppts;
}


static void GetIOSCameraDefaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, SETTING_DEVICE_UUID, "");
}

static void SaveIOSCameraInput(void *data, obs_data_t *settings)
{
    IOSCameraInput *input = reinterpret_cast<IOSCameraInput*>(data);
    
    blog(LOG_INFO, "SAVE");
    
    input->loadSettings(settings);
}

static void UpdateIOSCameraInput(void *data, obs_data_t *settings)
{
    IOSCameraInput *input = reinterpret_cast<IOSCameraInput*>(data);

    // Clear the video frame when a setting changes
    obs_source_output_video(input->source, NULL);

    // Connect to the device
    auto uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);
    input->connectToDevice(uuid);
}

void RegisterIOSCameraSource()
{
	obs_source_info info = {};
	info.id              = "ios-camera-source";
	info.type            = OBS_SOURCE_TYPE_INPUT;
	info.output_flags    = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;
	info.get_name        = GetIOSCameraInputName;
	
    info.create          = CreateIOSCameraInput;
	info.destroy         = DestroyIOSCameraInput;
    
    info.deactivate      = DeactivateIOSCameraInput;
    info.activate        = ActivateIOSCameraInput;
    
    info.get_defaults    = GetIOSCameraDefaults;
    info.get_properties  = GetIOSCameraProperties;
    info.save            = SaveIOSCameraInput;
    info.update          = UpdateIOSCameraInput;
	obs_register_source(&info);
}
