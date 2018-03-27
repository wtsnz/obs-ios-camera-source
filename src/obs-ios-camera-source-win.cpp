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

#include "ffmpeg-decode.h"

#define TEXT_INPUT_NAME obs_module_text("OBSIOSCamera.Title")

class Decoder {
	struct ffmpeg_decode decode;

public:
	inline Decoder()  {memset(&decode, 0, sizeof(decode));}
	inline ~Decoder() {ffmpeg_decode_free(&decode);}

	inline operator ffmpeg_decode*() {return &decode;}
	inline ffmpeg_decode *operator->() {return &decode;}
};

class IOSCameraInput: public portal::PortalDelegate {
public:

    obs_source_t *source;
	portal::Portal		 portal;
	Decoder      video_decoder;
	bool         active = false;
	obs_source_frame frame;

	inline IOSCameraInput(obs_source_t *source_, obs_data_t *settings)
		: source         (source_)
	{
		memset(&frame, 0, sizeof(frame));
		
		portal.startListeningForDevices();
        portal.delegate = this;
		active = true;

		blog(LOG_INFO, "[obs-ios-camera-source] Started listening for devices");
	}

	inline ~IOSCameraInput()
	{
		portal.stopListeningForDevices();
	}
    
    void portalDeviceDidReceivePacket(std::vector<char> packet) {
        unsigned char *data = (unsigned char *) packet.data();
        long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();        
        OnEncodedVideoData(AV_CODEC_ID_H264, data, packet.size(), now);
    }

	void OnEncodedVideoData(enum AVCodecID id,
			unsigned char *data, size_t size, long long ts)
	{
		if (!ffmpeg_decode_valid(video_decoder)) {
			if (ffmpeg_decode_init(video_decoder, id) < 0) {
				blog(LOG_WARNING, "Could not initialize video decoder");
				return;
			}
		}

		bool got_output;
		bool success = ffmpeg_decode_video(video_decoder, data, size, &ts,
				&frame, &got_output);
		if (!success) {
			blog(LOG_WARNING, "Error decoding video");
			return;
		}

		if (got_output) {
			frame.timestamp = (uint64_t)ts * 100;
			//if (flip)
				//frame.flip = !frame.flip;
	#if LOG_ENCODED_VIDEO_TS
			blog(LOG_DEBUG, "video ts: %llu", frame.timestamp);
	#endif
			obs_source_output_video(source, &frame);
		}
	}

};

static const char *GetIOSCameraInputName(void*)
{
	return TEXT_INPUT_NAME;
}

static void *CreateIOSCameraInput(obs_data_t *settings, obs_source_t *source)
{
	IOSCameraInput *cameraInput = nullptr;

	try {
		cameraInput = new IOSCameraInput(source, settings);
	} catch (const char *error) {
		blog(LOG_ERROR, "Could not create device '%s': %s", obs_source_get_name(source), error);
	}

	return cameraInput;
}

static void DestroyIOSCameraInput(void *data)
{
	delete reinterpret_cast<IOSCameraInput*>(data);
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
	// info.update          = UpdateDShowInput;
	// info.get_defaults    = GetDShowDefaults;
	// info.get_properties  = GetDShowProperties;
	obs_register_source(&info);
}
