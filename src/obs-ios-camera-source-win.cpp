#include <obs-module.h>
// #include <obs-frontend-api.h>
// #include <QAction>
// #include <QMainWindow>
// #include <QTimer>
#include <chrono>
#include <Portal.hpp>
#include <usbmuxd.h>
#include <obs-avc.h>
#include "ffmpeg-decode.h"

#define TEXT_INPUT_NAME obs_module_text("iOS Camera")

int avc_nalu_type(const uint8_t *data, size_t size)
{
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    int type;
    
    nal_start = obs_avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++));
        
        if (nal_start == end)
            break;
        
        type = nal_start[0] & 0x1F;
        
        return type;
        
//        if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE)
//            return (type == OBS_NAL_SLICE_IDR);
//
//        nal_end = obs_avc_find_startcode(nal_start, end);
//        nal_start = nal_end;
    }
    
    return 0;
}

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

		blog(LOG_INFO, "Created plugin!!!");

		portal.startListeningForDevices();
        portal.delegate = this;
		active = true;

		//OnEncodedVideoData(AV_CODEC_ID_H264, data, size, startTime);
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
	// SetLogCallback(DShowModuleLogCallback, nullptr);

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


// bool obs_module_load(void) {

//     obs_register_source(&usb_video_source_info);
//     // Loading finished
//     blog(LOG_INFO, "module loaded!");

//     return true;
// }

// void obs_module_unload() {
//     blog(LOG_INFO, "goodbye!");
// }

// struct usb_video_source {
// 	obs_source_t *source;
// 	// os_event_t   *stop_signal;

// 	// left_right::left_right<av_video_info> video_info;

// 	// FourCharCode fourcc;
// 	// video_format video_format;

//     obs_source_frame frame;

// 	// pthread_t    thread;

//     // dispatch_queue_t queue;
//     // USBReceiver *receiver;
//     // OBSUSBFrameSourceDelegate *usbFrameSourceDelegate;

// 	bool         initialized;
// };

// static void *random_create(obs_data_t *settings, obs_source_t *source)
// {
//     blog(LOG_INFO, "Create the source");

// 	struct usb_video_source *rt = (usb_video_source *)bzalloc(sizeof(struct usb_video_source));

//     blog(LOG_INFO, "Initialised");

// 	rt->initialized = true;

// 	UNUSED_PARAMETER(settings);
// 	UNUSED_PARAMETER(source);
// 	return rt;
// }


// static const char *random_getname(void *unused)
// {
//     blog(LOG_INFO, "getting name");
// 	UNUSED_PARAMETER(unused);
// 	return "iPhone Camera";
// }

// static void random_destroy(void *data)
// {
// //    auto usbVideoSource = static_cast<usb_video_source *>(data);
// //    auto *frame = &usb_video_source->frame;

// //    delete frame;
// }

