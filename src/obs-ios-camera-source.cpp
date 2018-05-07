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
//#include "VideoToolboxVideoDecoder.h"

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")

#define SETTING_DISCONNECT_WHEN_HIDDEN "setting_deactivate_when_not_showing"
#define SETTING_DEVICE_UUID "setting_device_uuid"

class IOSCameraInput: public portal::PortalDelegate
{
  public:
	obs_source_t *source;
	portal::Portal portal;
	bool active = false;
	obs_source_frame frame;
    
    int devicePort = 0;
    bool disconnectWhenDeactivated = false;
    
//    VideoToolboxDecoder decoder;
    FFMpegVideoDecoder decoder;
    
	inline IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
        : source(source_), portal(this, 1000)
	{
        UNUSED_PARAMETER(settings);
        
        blog(LOG_INFO, "Creating instance of plugin!");
        
		memset(&frame, 0, sizeof(frame));

//        blog(LOG_INFO, "Port %i", port);
        
        loadSettings(settings);
        
//        portal.delegate = this;
		active = true;
        
        decoder.source = source;
        decoder.Init();
        
        obs_source_set_async_unbuffered(source, true);
        
		blog(LOG_INFO, "Started listening for devices");
	}

	inline ~IOSCameraInput()
	{
		portal.stopListeningForDevices();
	}

    void activate() {
        blog(LOG_INFO, "Activating");
        
        if (disconnectWhenDeactivated) {
            portal.startListeningForDevices();
            portal.connectAllDevices();
        }
        
    }
    
    void deactivate() {
        blog(LOG_INFO, "Deactivating");
        
        if (disconnectWhenDeactivated) {
            portal.disconnectAllDevices();
        }
    }
    
    void loadSettings(obs_data_t *settings) {
        disconnectWhenDeactivated = obs_data_get_bool(settings, SETTING_DISCONNECT_WHEN_HIDDEN);
        
        auto device_uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);
        connectToDevice(device_uuid);
    }
    
    void connectToDevice(std::string uuid) {
        
        // Find device
        auto devices = portal.getDevices();
        
        int index = 0;
        std::for_each(devices.begin(), devices.end(), [this, uuid, &index](std::map<int, portal::Device::shared_ptr>::value_type &deviceMap)
                      {
                          // Add the device name to the list
                          auto _uuid = deviceMap.second->uuid();
                          
//                          uuid.
                          if (_uuid == uuid) {
                              portal.connectToDevice(deviceMap.second);
                          }
                          
                          index++;
                      }
                      );
        
    }
    
	void portalDeviceDidReceivePacket(std::vector<char> packet)
	{
        try
        {
            this->decoder.Input(packet);
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
    
    void portalDidUpdateDeviceList(std::map<int, portal::Device::shared_ptr>)
    {
        // Update OBS Settings
//        source->
        blog(LOG_INFO, "Updated device list");
    }
    
};

#pragma mark - Settings Config

static bool properties_selected_device_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *data)
{
//    auto cameraInput =  reinterpret_cast<IOSCameraInput*>(data);

    auto uuid = obs_data_get_string(data, "device");
    
    // Connect to UUID
//    if (cameraInput->source != NULL) {
//        cameraInput->connectToDevice(uuid);
//    }
    
//    NSString *uid = get_string(settings, "device");
//    AVCaptureDevice *dev = [AVCaptureDevice deviceWithUniqueID:uid];
//
//    NSString *name = get_string(settings, "device_name");
//    bool dev_list_updated = update_device_list(p, uid, name,
//                                               !dev && uid.length);
//
//    p = obs_properties_get(props, "preset");
//    bool preset_list_changed = check_preset(dev, p, settings);
//    bool autoselect_changed  = autoselect_preset(dev, settings);
//
//    config_helper conf{settings};
//    bool res_changed = update_resolution_property(props, conf);
//    bool fps_changed = update_frame_rate_property(props, conf);
//    bool if_changed  = update_input_format_property(props, conf);
//
//    return preset_list_changed || autoselect_changed || dev_list_updated
//    || res_changed || fps_changed || if_changed;
}

static bool refresh_devices(obs_properties_t *props, obs_property_t *p, void *data)
{
    auto cameraInput =  reinterpret_cast<IOSCameraInput*>(data);
    
    cameraInput->portal.reloadDeviceList();
    auto devices = cameraInput->portal.getDevices();
    
    obs_property_t *dev_list = obs_properties_get(props, SETTING_DEVICE_UUID);
    obs_property_list_clear(dev_list);
    
    obs_property_list_add_string(dev_list, "", "");
    
    int index = 1;
    std::for_each(devices.begin(), devices.end(), [dev_list, &index](std::map<int, portal::Device::shared_ptr>::value_type &deviceMap)
                  {
                      // Add the device name to the list
                      auto name = deviceMap.second->getProductId().c_str();
                      auto uuid = deviceMap.second->uuid().c_str();
                      obs_property_list_add_string(dev_list, uuid, uuid);
                      
                      // Disable the row if the device is selected as we can only
                      // connect to one device to one source.
//                      auto isConnected = deviceMap.second->isConnected();
//                      obs_property_list_item_disable(dev_list, index, isConnected);
                      
                      index++;
                  }
    );
    
    return true;
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
    
    obs_properties_add_bool(ppts, SETTING_DISCONNECT_WHEN_HIDDEN, "Disconnect device when this source is hidden");
//    obs_properties_add_int(ppts, SETTING_DEVICE_PORT, "Device ID", 1000, 5000, 1);
    
    obs_property_t *dev_list = obs_properties_add_list(ppts, SETTING_DEVICE_UUID,
                                                       "iOS Device",
                                                       OBS_COMBO_TYPE_LIST,
                                                       OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(dev_list, "", "");

    
    refresh_devices(ppts, dev_list, data);
    
    obs_properties_add_button(ppts, "setting_refresh_devices", "Refresh Devices", refresh_devices);
//    obs_properties_add_button(ppts, "dsfdsssf", "Connect to Device", nil);

//    for (AVCaptureDevice *dev in [AVCaptureDevice
//                                  devices]) {
//        if ([dev hasMediaType: AVMediaTypeVideo] ||
//            [dev hasMediaType: AVMediaTypeMuxed]) {
//            obs_property_list_add_string(dev_list,
//                                         dev.localizedName.UTF8String,
//                                         dev.uniqueID.UTF8String);
//        }
//    }
    
//    obs_property_set_modified_callback(dev_list, properties_device_changed);
    
    return ppts;
}


static void GetIOSCameraDefaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, SETTING_DISCONNECT_WHEN_HIDDEN, false);
    obs_data_set_default_string(settings, SETTING_DEVICE_UUID, "");
//    obs_data_set_default_int(settings, SETTING_DEVICE_PORT, 1234);
}

static void SaveIOSCameraInput(void *data, obs_data_t *settings)
{
    IOSCameraInput *input = reinterpret_cast<IOSCameraInput*>(data);
    
    blog(LOG_INFO, "SAVE");
    
    input->loadSettings(settings);
//    input->portal.setDevicePort(input->devicePort);
}

static void UpdateIOSCameraInput(void *data, obs_data_t *settings)
{
    IOSCameraInput *input = reinterpret_cast<IOSCameraInput*>(data);
    
    blog(LOG_INFO, "SAVE");
    
    auto uuid = obs_data_get_string(settings, "device");

    input->connectToDevice(uuid);
//    input->loadSettings(settings);
    
    // Refresh devices (to update the connection states)
//    refresh_devices(settings, data);
}

void RegisterIOSCameraSource()
{
	obs_source_info info = {};
	info.id              = "ios-camera-source";
	info.type            = OBS_SOURCE_TYPE_INPUT;
	info.output_flags    = OBS_SOURCE_ASYNC_VIDEO;
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
