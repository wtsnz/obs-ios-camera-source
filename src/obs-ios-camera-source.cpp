/*
 obs-ios-camera-source
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

#include "obs-ios-camera-source.h"

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")
#define SETTING_DEVICE_UUID "setting_device_uuid"
#define SETTING_DEVICE_UUID_NONE_VALUE "null"
#define SETTING_PROP_LATENCY "latency"
#define SETTING_PROP_LATENCY_NORMAL 0
#define SETTING_PROP_LATENCY_LOW 1
#define SETTING_PROP_HARDWARE_DECODER "setting_use_hw_decoder"
#define SETTING_PROP_DISCONNECT_ON_INACTIVE "setting_disconnect_on_inactive"

IOSCameraInput::IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
	: deviceManager(), source(source_), settings(settings)
{
	blog(LOG_INFO, "Creating instance of plugin!");

#ifdef __APPLE__
	videoToolboxVideoDecoder.source = source_;
	videoToolboxVideoDecoder.Init();
#endif

	ffmpegVideoDecoder.source = source_;
	ffmpegVideoDecoder.Init();

	audioDecoder.source = source_;
	audioDecoder.Init();

	videoDecoder = &ffmpegVideoDecoder;

	state = State();
	active = true;
	loadSettings(settings);

	deviceManager.onDeviceManagerDidUpdateDeviceListCallback =
		[this](auto devices) {
			this->deviceManagerDidUpdateDeviceList(devices);
		};
	deviceManager.onDeviceManagerDidChangeStateCallback =
		[this](auto state) {
			this->deviceManagerDidChangeState(state);
		};
	deviceManager.onDeviceManagerDidAddDeviceCallback =
		[this](auto device) {
			this->deviceManagerDidAddDevice(device);
		};
	deviceManager.onDeviceManagerDidRemoveDeviceCallback =
		[this](auto device) {
			this->deviceManagerDidRemoveDevice(device);
		};

	deviceManager.start();
};

void IOSCameraInput::deviceManagerDidUpdateDeviceList(
	std::map<std::string, portal::Device::shared_ptr> devices)
{
	blog(LOG_INFO, "Updated device list");
	auto cameraDevices = devicesFromPortal(devices);
	state.devices = cameraDevices;

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
	if (state.devices.size() == 1) {

		auto device = state.devices[0];

		auto isFirstTimeConnectingToDevice = state.selectedDeviceUUID ==
						     std::nullopt;
		auto isDeviceConnected =
			connectionControllers[device.uuid]->getState() ==
			portal::DeviceConnection::State::Connected;

		auto isThisDeviceTheSameAsLastConnectedDevice =
			device.uuid.compare(
				state.selectedDeviceUUID.value_or("null")) == 0;

		if (isFirstTimeConnectingToDevice ||
		    (isThisDeviceTheSameAsLastConnectedDevice &&
		     !isDeviceConnected)) {

			setDeviceUUID(device.uuid);

			// Set the setting so that the UI in OBS Studio is updated
			obs_data_set_string(this->settings, SETTING_DEVICE_UUID,
					    device.uuid.c_str());
		}

	} else {
		// User will have to configure the plugin manually when more than one device is plugged in
		// due to the fact that multiple instances of the plugin can't subscribe to device events...

		connectToDevice();
	}
}

void IOSCameraInput::deviceManagerDidChangeState(
	portal::DeviceManager::State state)
{
	blog(LOG_INFO, "deviceManagerDidChangeState %i", state);
	std::cout << "DeviceManager::didChangeState: " << state << std::endl;
}

void IOSCameraInput::deviceManagerDidAddDevice(portal::Device::shared_ptr device)
{
	std::cout << "Did Add device " << device->uuid() << std::endl;

	// Create the connection, and the connection manager, but don't start anything just yet
	auto deviceConnection =
		std::make_shared<portal::DeviceConnection>(device, 2345);
	auto deviceConnectionController =
		std::make_shared<DeviceApplicationConnectionController>(
			deviceConnection);
	connectionControllers[device->uuid()] = deviceConnectionController;

	// Setup the callbacks

	deviceConnectionController->onProcessPacketCallback = [this](auto packet) {
		try {
			switch (packet.type) {
			case 101: // Video Packet
				this->videoDecoder->Input(packet.data, packet.type, packet.tag);
				break;
			case 102: // Audio Packet
				this->audioDecoder.Input(packet.data, packet.type, packet.tag);
			default:
				break;
			}
		} catch (...) {
			// This isn't great, but I haven't been able to figure out what is causing
			// the exception that happens when
			//   the phone is plugged in with the app open
			//   OBS Studio is launched with the iOS Camera plugin ready
			// This also doesn't happen _all_ the time. Which makes this 'fun'..
			blog(LOG_INFO, "Exception caught...");
		}
	};
}

void IOSCameraInput::deviceManagerDidRemoveDevice(
	portal::Device::shared_ptr device)
{
	std::cout << "Did Remove device " << device->uuid() << std::endl;

	if (auto deviceConnectionController =
		    connectionControllers[device->uuid()]) {
		deviceConnectionController->disconnect();
		connectionControllers[device->uuid()] = nullptr;
	}
}

IOSCameraInput ::~IOSCameraInput()
{

}

void IOSCameraInput::activate()
{
	blog(LOG_INFO, "Activating");
	active = true;

	connectToDevice();
}

void IOSCameraInput::deactivate()
{
	blog(LOG_INFO, "Deactivating");
	active = false;

	connectToDevice();
}

void IOSCameraInput::loadSettings(obs_data_t *settings)
{
	disconnectOnInactive = obs_data_get_bool(
		settings, SETTING_PROP_DISCONNECT_ON_INACTIVE);

	auto device_uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);
	state.selectedDeviceUUID = device_uuid;

	blog(LOG_INFO, "Loaded Settings");

	setDeviceUUID(device_uuid);
}

void IOSCameraInput::setDeviceUUID(std::string uuid)
{
	state.lastSelectedDeviceUUID = state.selectedDeviceUUID;

	if (uuid == SETTING_DEVICE_UUID_NONE_VALUE) {
		state.selectedDeviceUUID = std::nullopt;
	} else {
		state.selectedDeviceUUID = uuid;
	}

	connectToDevice();
}

void IOSCameraInput::reconnectToDevice()
{
	connectToDevice();
}

void IOSCameraInput::resetDecoder()
{
	// flush the decoders
	ffmpegVideoDecoder.Flush();
#ifdef __APPLE__
	videoToolboxVideoDecoder.Flush();
#endif

	// Clear the video frame when a setting changes
	obs_source_output_video(source, NULL);
}

void IOSCameraInput::connectToDevice()
{
	blog(LOG_DEBUG, "Connecting to device: %s",
	     state.selectedDeviceUUID.value_or("none").c_str());

	auto isConnectingToDifferentDevice = state.lastSelectedDeviceUUID !=
					     state.selectedDeviceUUID;

	// If there is no currently selected device, disconnect from all
	// connection controllers
	if (!state.selectedDeviceUUID.has_value()) {
		for (const auto &[uuid, connectionController] :
		     connectionControllers) {
			if (connectionController != nullptr) {
				connectionController->disconnect();
			}
		}

		// Clear the video frame when a setting changes
		resetDecoder();
		return;
	}

	// https://stackoverflow.com/questions/44217316/how-do-i-use-stdoptional-in-c
	// Apple compiler hasn't implemented std::optional.value() in < macos 10.14,
	// work around this by fetching the value by * method.
	std::string selectedUUID = *state.selectedDeviceUUID;

	// Disconnect all connection controllers
	for (const auto &[uuid, connectionController] : connectionControllers) {
		if (connectionController != nullptr) {
			connectionController->disconnect();
		}
	}

	if (isConnectingToDifferentDevice) {
		resetDecoder();
	}

	auto shouldConnect = !(disconnectOnInactive == true && active == false);

	// Then connect to the selected device if the plugin is active, or inactive and connected on inactive.
	for (const auto &[uuid, connectionController] : connectionControllers) {
		if (connectionController != nullptr) {
			if (uuid == selectedUUID && shouldConnect) {
				blog(LOG_DEBUG,
				     "Starting connection controller");
				connectionController->start();
			}
		}
	}
}

#pragma mark - Settings Config

static bool refresh_devices(obs_properties_t *props, obs_property_t *p,
			    void *data)
{
	UNUSED_PARAMETER(p);

	auto cameraInput = reinterpret_cast<IOSCameraInput *>(data);

	obs_property_t *dev_list = obs_properties_get(props, SETTING_DEVICE_UUID);
	obs_property_list_clear(dev_list);

	obs_property_list_add_string(dev_list, "None",
				     SETTING_DEVICE_UUID_NONE_VALUE);

	int index = 1;
	std::for_each(
		cameraInput->state.devices.begin(),
		cameraInput->state.devices.end(),
		[dev_list, &index](IOSCameraInput::MobileCameraDevice &device) {

            auto uuid = device.uuid.c_str();
			auto name = device.name.c_str();
			obs_property_list_add_string(dev_list, name, uuid);

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

static bool reconnect_to_device(obs_properties_t *props, obs_property_t *p,
				void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);

	auto cameraInput = reinterpret_cast<IOSCameraInput *>(data);
	cameraInput->reconnectToDevice();

	return false;
}

#pragma mark - Plugin Callbacks

static const char *GetIOSCameraInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void UpdateIOSCameraInput(void *data, obs_data_t *settings);

static void *CreateIOSCameraInput(obs_data_t *settings, obs_source_t *source)
{
	IOSCameraInput *cameraInput = nullptr;

    try
    {
        cameraInput = new IOSCameraInput(source, settings);
        UpdateIOSCameraInput(cameraInput, settings);
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
	auto cameraInput = reinterpret_cast<IOSCameraInput *>(data);
	cameraInput->deactivate();
}

static void ActivateIOSCameraInput(void *data)
{
	auto cameraInput = reinterpret_cast<IOSCameraInput *>(data);
	cameraInput->activate();
}

static obs_properties_t *GetIOSCameraProperties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *ppts = obs_properties_create();

	obs_property_t *dev_list = obs_properties_add_list(
		ppts, SETTING_DEVICE_UUID, "iOS Device", OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(dev_list, "", "");

	refresh_devices(ppts, dev_list, data);

	obs_properties_add_button(ppts, "setting_refresh_devices",
				  "Refresh Devices", refresh_devices);
	obs_properties_add_button(ppts, "setting_button_connect_to_device",
				  "Reconnect to Device", reconnect_to_device);

	obs_property_t *latency_modes = obs_properties_add_list(
		ppts, SETTING_PROP_LATENCY,
		obs_module_text("OBSIOSCamera.Settings.Latency"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(
		latency_modes,
		obs_module_text("OBSIOSCamera.Settings.Latency.Normal"),
		SETTING_PROP_LATENCY_NORMAL);
	obs_property_list_add_int(
		latency_modes,
		obs_module_text("OBSIOSCamera.Settings.Latency.Low"),
		SETTING_PROP_LATENCY_LOW);

#ifdef __APPLE__
	obs_properties_add_bool(
		ppts, SETTING_PROP_HARDWARE_DECODER,
		obs_module_text("OBSIOSCamera.Settings.UseHardwareDecoder"));
#endif

	obs_properties_add_bool(
		ppts, SETTING_PROP_DISCONNECT_ON_INACTIVE,
		obs_module_text("OBSIOSCamera.Settings.DisconnectOnInactive"));

	return ppts;
}

static void GetIOSCameraDefaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, SETTING_DEVICE_UUID, "");
	obs_data_set_default_int(settings, SETTING_PROP_LATENCY,
				 SETTING_PROP_LATENCY_LOW);
#ifdef __APPLE__
	obs_data_set_default_bool(settings, SETTING_PROP_HARDWARE_DECODER,
				  false);
#endif
	obs_data_set_default_bool(settings, SETTING_PROP_DISCONNECT_ON_INACTIVE,
				  false);
}

static void SaveIOSCameraInput(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

static void UpdateIOSCameraInput(void *data, obs_data_t *settings)
{
	IOSCameraInput *input = reinterpret_cast<IOSCameraInput *>(data);

	// Connect to the device
	auto uuid = obs_data_get_string(settings, SETTING_DEVICE_UUID);
	input->setDeviceUUID(uuid);

	const bool is_unbuffered =
		(obs_data_get_int(settings, SETTING_PROP_LATENCY) ==
		 SETTING_PROP_LATENCY_LOW);
	obs_source_set_async_unbuffered(input->source, is_unbuffered);

#ifdef __APPLE__
	bool useHardwareDecoder =
		obs_data_get_bool(settings, SETTING_PROP_HARDWARE_DECODER);

	if (useHardwareDecoder) {
		input->videoDecoder = &input->videoToolboxVideoDecoder;
	} else {
		input->videoDecoder = &input->ffmpegVideoDecoder;
	}
#endif

	input->disconnectOnInactive = obs_data_get_bool(
		settings, SETTING_PROP_DISCONNECT_ON_INACTIVE);
}

void RegisterIOSCameraSource()
{
	obs_source_info info = {};
	info.id = "ios-camera-source";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO;
	info.get_name = GetIOSCameraInputName;

	info.create = CreateIOSCameraInput;
	info.destroy = DestroyIOSCameraInput;

	info.deactivate = DeactivateIOSCameraInput;
	info.activate = ActivateIOSCameraInput;

	info.get_defaults = GetIOSCameraDefaults;
	info.get_properties = GetIOSCameraProperties;
	info.save = SaveIOSCameraInput;
	info.update = UpdateIOSCameraInput;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	obs_register_source(&info);
}
