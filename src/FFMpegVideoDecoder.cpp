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

#include "FFMpegVideoDecoder.h"
#include <util/platform.h>

FFMpegVideoDecoder::FFMpegVideoDecoder()
{
	memset(&video_frame, 0, sizeof(video_frame));
}

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
    this->Shutdown();
    // Free the video decoder.
    ffmpeg_decode_free(video_decoder);
}

void FFMpegVideoDecoder::Init()
{
    // Start the thread.
    this->start();
}

void FFMpegVideoDecoder::Flush()
{
    // Clear the queue
    while(this->mQueue.size() > 0) {
        delete this->mQueue.remove();
    }

    std::lock_guard<std::mutex> lock(mMutex);
    ffmpeg_decode_flush(video_decoder);
}

void FFMpegVideoDecoder::Drain()
{
    // Drain the queue
}

void FFMpegVideoDecoder::Shutdown()
{
    mQueue.stop();
    this->join();
}

void FFMpegVideoDecoder::Input(std::vector<char> packet, int type, int tag)
{
    // Create a new packet item and enqueue it.
    PacketItem *item = new PacketItem(packet, type, tag);
    this->mQueue.add(item);
}

static const char *ffmpeg_decode_video_name = "obs_camera_ffmpeg_decode_video";
void FFMpegVideoDecoder::processPacketItem(PacketItem *packetItem)
{
	std::lock_guard<std::mutex> lock(mMutex);
	auto packet = packetItem->getPacket();
	unsigned char *data = (unsigned char *)packet.data();
	const enum AVCodecID detected_codec =
		ffmpeg_detect_video_codec(data, packet.size());

	if (ffmpeg_decode_valid(video_decoder) &&
	    detected_codec != AV_CODEC_ID_NONE &&
	    video_decoder->codec->id != detected_codec) {
		blog(LOG_INFO, "FFmpeg: switching video decoder from %s to %s",
		     avcodec_get_name(video_decoder->codec->id),
		     avcodec_get_name(detected_codec));
		ffmpeg_decode_free(video_decoder);
	}

	if (!ffmpeg_decode_valid(video_decoder)) {
		const enum AVCodecID codec = detected_codec != AV_CODEC_ID_NONE
						      ? detected_codec
						      : AV_CODEC_ID_H264;
		int result;
		if (codec == AV_CODEC_ID_HEVC) {
			result = ffmpeg_decode_init_hardware(
				video_decoder, codec, AV_HWDEVICE_TYPE_CUDA);
		} else {
			result = ffmpeg_decode_init(video_decoder, codec);
		}

		if (result < 0) {
			blog(LOG_WARNING, "Could not initialize %s video decoder",
			     avcodec_get_name(codec));
			return;
		}
		if (video_decoder->hardware_active) {
			blog(LOG_INFO,
			     "FFmpeg: initialized %s video decoder using %s hardware acceleration",
			     avcodec_get_name(codec),
			     av_hwdevice_get_type_name(
				     video_decoder->hardware_device_type));
		} else {
			blog(LOG_INFO, "FFmpeg: initialized %s software video decoder",
			     avcodec_get_name(codec));
		}
	}

    if (packetItem->getType() == 101) {
        profile_start(ffmpeg_decode_video_name);

		bool first = true;
		bool got_output = false;
		do {
			long long ts = (long long)os_gettime_ns();
			bool success = ffmpeg_decode_video(
				video_decoder, first ? data : nullptr,
				first ? packet.size() : 0, &ts, &video_frame,
				&got_output);
			first = false;
			if (!success) {
				const bool hardware_failed =
					video_decoder->hardware_active;
				blog(LOG_WARNING,
				     "Error decoding %s video packet%s",
				     avcodec_get_name(video_decoder->codec->id),
				     hardware_failed
					     ? "; falling back to software"
					     : "");
				if (hardware_failed) {
					const enum AVCodecID codec =
						video_decoder->codec->id;
					ffmpeg_decode_free(video_decoder);
					ffmpeg_decode_init(video_decoder, codec);
				}
				break;
			}
			if (got_output && source != nullptr) {
				video_frame.timestamp = ts >= 0
							? (uint64_t)ts
							: os_gettime_ns();
				obs_source_output_video(source, &video_frame);
			}
		} while (got_output);

		profile_end(ffmpeg_decode_video_name);
	}
}

void *FFMpegVideoDecoder::run() {

    while (shouldStop() == false) {

        PacketItem *item = (PacketItem *)mQueue.remove();

        if (item != NULL) {
            this->processPacketItem(item);
            delete item;
        }

        // Check queue lengths

        const int queueSize = mQueue.size();
        if (queueSize > 5) {
            blog(LOG_WARNING, "FFMpeg: Decoding queue overloaded. %d frames behind. Please use a lower quality setting.", queueSize);

            while (mQueue.size() > 0) {
                PacketItem *item = (PacketItem *)mQueue.remove();
                if (item != NULL) {
                    blog(LOG_INFO, "FFMpeg: dropping packet type=%d tag=%d size=%d", item->getType(), item->getTag(), item->size());
                    delete item;
                }
            }
        }
    }
    return NULL;
}
