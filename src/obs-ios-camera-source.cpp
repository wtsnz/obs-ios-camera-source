/*
 obs-ios-camera-source
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

#include <obs-module.h>
#include <chrono>
#include <Portal.hpp>
#include <usbmuxd.h>
#include <obs-avc.h>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "FFMpegVideoDecoder.h"
#include "FFMpegAudioDecoder.h"
#ifdef __APPLE__
    #include "VideoToolboxVideoDecoder.h"
#endif

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")

#define SETTING_DEVICE_UUID "setting_device_uuid"
#define SETTING_DEVICE_UUID_NONE_VALUE "null"
#define SETTING_PROP_LATENCY "latency"
#define SETTING_PROP_LATENCY_NORMAL 0
#define SETTING_PROP_LATENCY_LOW 1

#define SETTING_PROP_HARDWARE_DECODER "setting_use_hw_decoder"

#define SETTING_PROP_DISCONNECT_ON_INACTIVE "setting_disconnect_on_inactive"

class IOSCameraInput: public portal::PortalDelegate
{
public:
    obs_source_t *source;
    obs_data_t *settings;

    std::atomic_bool active = false;
    std::atomic_bool disconnectOnInactive = false;
    obs_source_frame frame;
    std::string deviceUUID;

    std::shared_ptr<portal::Portal> sharedPortal;
    portal::Portal portal;

    std::atomic_bool stopping =  false;
    std::thread connect_thread;
    std::mutex mutex;
    std::condition_variable condition_variable;
    std::atomic_bool force_reconnect = false;

    VideoDecoder *videoDecoder;
#ifdef __APPLE__
    VideoToolboxDecoder videoToolboxVideoDecoder;
#endif
    FFMpegVideoDecoder ffmpegVideoDecoder;
    FFMpegAudioDecoder audioDecoder;

    IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
    : source(source_), settings(settings), portal(this)
    {
        blog(LOG_INFO, "Creating instance of plugin!");

        memset(&frame, 0, sizeof(frame));

        /// In order for the internal Portal Delegates to work there
        /// must be a shared_ptr to the instance of Portal.
        ///
        /// We create a shared pointer to the heap allocated Portal
        /// instance, and wrap it up in a sharedPointer with a deleter
        /// that doesn't do anything (this is handled automatically with
        /// the class)
        auto null_deleter = [](portal::Portal *portal) { UNUSED_PARAMETER(portal); };
        auto portalReference = std::shared_ptr<portal::Portal>(&portal, null_deleter);
        sharedPortal = portalReference;

#ifdef __APPLE__
        videoToolboxVideoDecoder.source = source;
        videoToolboxVideoDecoder.Init();
#endif

        ffmpegVideoDecoder.source = source;
        ffmpegVideoDecoder.Init();

        audioDecoder.source = source;
        audioDecoder.Init();

        videoDecoder = &ffmpegVideoDecoder;

        loadSettings(settings);
        active = true;
        connect_thread = std::thread(&IOSCameraInput::connect_worker, this);
    }

    ~IOSCameraInput()
    {
        portal.stopListeningForDevices();
        stopping = true;
        signalConnect();
        connect_thread.join();
    }

    void connect_worker() {
        bool last_active = active;
        std::string last_device_uuid;
        bool last_disconnect_on_inactive = disconnectOnInactive;

        while (!stopping) {
            std::unique_lock<std::mutex> lock(mutex);
            blog(LOG_DEBUG, "connect_worker: Running check");

            bool should_disconnect = disconnectOnInactive && !active || deviceUUID == "";

            if (force_reconnect || should_disconnect) {
                blog(LOG_DEBUG, "connect_worker: Disconnecting");
                disconnectFromDevice();
                force_reconnect = false;
            }

            if (active || !disconnectOnInactive) {
                blog(LOG_DEBUG, "connect_worker: Connecting");
                connectToDevice();
            }

            if (last_active == active && last_device_uuid == deviceUUID && last_disconnect_on_inactive == disconnectOnInactive) {
                blog(LOG_DEBUG, "connect_worker: Waiting");
                condition_variable.wait_for(lock, std::chrono::seconds(1));
            }

            last_active = active;
            last_device_uuid = deviceUUID;
            last_disconnect_on_inactive = disconnectOnInactive;

            lock.unlock();
        }
    }

    void signalConnect() {
        condition_variable.notify_all();
    }

    void activate() {
        blog(LOG_INFO, "Activating");
        active = true;
        signalConnect();
    }

    void deactivate() {
        blog(LOG_INFO, "Deactivating");
        active = false;
        signalConnect();
    }

    void loadSettings(obs_data_t *settings) {
        auto device_uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);

        blog(LOG_INFO, "Loaded Settings: Connecting to device");

        setDeviceUUID(device_uuid);
    }

    void setDeviceUUID(std::string uuid) {
        std::scoped_lock lock(mutex);

        if (uuid == SETTING_DEVICE_UUID_NONE_VALUE) {
            deviceUUID = "";
        } else {
            deviceUUID = uuid;
        }

        signalConnect();
    }

    void reconnectToDevice()
    {
        if (deviceUUID.size() < 1) {
            return;
        }

        force_reconnect = true;
        signalConnect();
    }

    void disconnectFromDevice() {
        portal.disconnectDevice();

        // flush the decoders 
        ffmpegVideoDecoder.Flush();
#ifdef __APPLE__
        videoToolboxVideoDecoder.Flush();
#endif

        // Clear the video frame when a setting changes
        obs_source_output_video(source, NULL);
    }

    bool isConnectedTo(std::string uuid) {
        return (portal._device && portal._device->isConnected() && portal._device->uuid().compare(uuid) == 0);
    }

    void connectToDevice() {
        if (deviceUUID.size() == 0) {
            return;
        }

        if (isConnectedTo(deviceUUID)) {
            blog(LOG_DEBUG, "Already connected to the device. Skipping.");
            return;
        }

        disconnectFromDevice();

        // Find device
        auto devices = portal.getDevices();

        auto deviceElement = std::find_if(devices.begin(), devices.end(), [this](const auto &element) {
            return element.second->uuid().compare(deviceUUID) == 0;
        });

        if (deviceElement != devices.end()) {
            blog(LOG_DEBUG, "Connecting to device %s", deviceUUID.c_str());
            portal.connectToDevice(deviceElement->second);
        } else {
            blog(LOG_INFO, "No device found to connect for %s", deviceUUID.c_str());
        }
    }

    void portalDeviceDidReceivePacket(std::vector<char> packet, int type, int tag)
    {
        try
        {
            switch (type) {
                case 101: // Video Packet
                    this->videoDecoder->Input(packet, type, tag);
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
        if (deviceUUID.size() != 0) {
            return;
        }

        // Update OBS Settings
        blog(LOG_INFO, "Updated device list");

        /// If there is one device in the list, then we should attempt to connect to it.
        /// I would guess that this is the main use case - one device, and it's good to
        /// attempt to automatically connect in this case, and 'just work'.
        ///
        /// If there are multiple devices, then we can't just connect to all devices.
        /// We cannot currently detect if a device is connected to another instance of the
        /// plugin, so it's not safe to attempt to connect to any devices automatically
        /// as we could be connecting to a device that is currently connected elsewhere.
        /// Due to this, if there are multiple devices, we won't do anything and will let
        /// the user configure the instance of the plugin.
        if (deviceList.size() == 1) {
            for (const auto& [index, device] : deviceList) {
                auto uuid = device.get()->uuid();

                auto isFirstTimeConnectingToDevice = deviceUUID.size() == 0;
                auto isDeviceConnected = device.get()->isConnected();
                auto isThisDeviceTheSameAsThePreviouslyConnectedDevice = deviceUUID.compare(uuid) == 0;

                if (isFirstTimeConnectingToDevice || (isThisDeviceTheSameAsThePreviouslyConnectedDevice && !isDeviceConnected)) {

                    // Set the setting so that the UI in OBS Studio is updated
                    obs_data_set_string(this->settings, SETTING_DEVICE_UUID, uuid.c_str());

                    // Connect to the device
                    setDeviceUUID(uuid);
                }
            }
        } else {
            // User will have to configure the plugin manually when more than one device is plugged in
            // due to the fact that multiple instances of the plugin can't subscribe to device events...
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

    obs_property_list_add_string(dev_list, "None", SETTING_DEVICE_UUID_NONE_VALUE);

    int index = 1;
    std::for_each(devices.begin(), devices.end(), [dev_list, &index](std::map<int, portal::Device::shared_ptr>::value_type &deviceMap) {
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
    });

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

    obs_property_t* latency_modes = obs_properties_add_list(ppts, SETTING_PROP_LATENCY, obs_module_text("OBSIOSCamera.Settings.Latency"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    obs_property_list_add_int(latency_modes,
        obs_module_text("OBSIOSCamera.Settings.Latency.Normal"),
        SETTING_PROP_LATENCY_NORMAL);
    obs_property_list_add_int(latency_modes,
        obs_module_text("OBSIOSCamera.Settings.Latency.Low"),
        SETTING_PROP_LATENCY_LOW);

#ifdef __APPLE__
    obs_properties_add_bool(ppts, SETTING_PROP_HARDWARE_DECODER,
        obs_module_text("OBSIOSCamera.Settings.UseHardwareDecoder"));
#endif

    obs_properties_add_bool(ppts, SETTING_PROP_DISCONNECT_ON_INACTIVE,
        obs_module_text("OBSIOSCamera.Settings.DisconnectOnInactive"));

    return ppts;
}


static void GetIOSCameraDefaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, SETTING_DEVICE_UUID, "");
    obs_data_set_default_int(settings, SETTING_PROP_LATENCY, SETTING_PROP_LATENCY_LOW);
#ifdef __APPLE__
    obs_data_set_default_bool(settings, SETTING_PROP_HARDWARE_DECODER, false);
#endif
    obs_data_set_default_bool(settings, SETTING_PROP_DISCONNECT_ON_INACTIVE, false);
}

static void SaveIOSCameraInput(void *data, obs_data_t *settings)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(settings);
}

static void UpdateIOSCameraInput(void *data, obs_data_t *settings)
{
    IOSCameraInput *input = reinterpret_cast<IOSCameraInput*>(data);

    // Connect to the device
    auto uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);
    input->setDeviceUUID(uuid);

    const bool is_unbuffered =
        (obs_data_get_int(settings, SETTING_PROP_LATENCY) == SETTING_PROP_LATENCY_LOW);
    obs_source_set_async_unbuffered(input->source, is_unbuffered);

#ifdef __APPLE__
    bool useHardwareDecoder = obs_data_get_bool(settings, SETTING_PROP_HARDWARE_DECODER);

    if (useHardwareDecoder) {
        input->videoDecoder = &input->videoToolboxVideoDecoder;
    } else {
        input->videoDecoder = &input->ffmpegVideoDecoder;
    }
#endif

    input->disconnectOnInactive = obs_data_get_bool(settings, SETTING_PROP_DISCONNECT_ON_INACTIVE);
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
