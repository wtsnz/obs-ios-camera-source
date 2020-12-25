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
        this->mQueue.remove();
    }

    mMutex.lock();
    // Re-initialize the decoder
    ffmpeg_decode_free(video_decoder);
    mMutex.unlock();
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
    mMutex.lock();
    uint64_t cur_time = os_gettime_ns();
    if (!ffmpeg_decode_valid(video_decoder))
    {
        if (ffmpeg_decode_init(video_decoder, AV_CODEC_ID_H264) < 0)
        {
            blog(LOG_WARNING, "Could not initialize video decoder");
            return;
        }
    }

    auto packet = packetItem->getPacket();
    unsigned char *data = (unsigned char *)packet.data();
    long long ts = cur_time;

    if (packetItem->getType() == 101) {
        profile_start(ffmpeg_decode_video_name);

        bool got_output;
        bool success = ffmpeg_decode_video(video_decoder, data, packet.size(), &ts,
                                           &video_frame, &got_output);

        profile_end(ffmpeg_decode_video_name);
        if (!success)
        {
            blog(LOG_WARNING, "Error decoding video");
            mMutex.unlock();
            return;
        }

        if (got_output && source != NULL)
        {
            video_frame.timestamp = cur_time;
            obs_source_output_video(source, &video_frame);
        }
    }
    mMutex.unlock();
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

