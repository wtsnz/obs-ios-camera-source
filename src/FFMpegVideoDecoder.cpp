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

FFMpegVideoDecoder::FFMpegVideoDecoder()
{
    memset(&frame, 0, sizeof(frame));
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
}

void FFMpegVideoDecoder::Drain()
{
    // Drain the queue
}

void FFMpegVideoDecoder::Shutdown()
{
    this->join();
}

void FFMpegVideoDecoder::Input(std::vector<char> packet)
{
    // Create a new packet item and enqueue it.
    PacketItem *item = new PacketItem(packet);
    this->mQueue.add(item);
}

void FFMpegVideoDecoder::processPacketItem(PacketItem *packetItem)
{
    auto packet = packetItem->getPacket();
    
    unsigned char *data = (unsigned char *)packet.data();
    long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (!ffmpeg_decode_valid(video_decoder))
    {
        if (ffmpeg_decode_init(video_decoder, AV_CODEC_ID_H264) < 0)
        {
            blog(LOG_WARNING, "Could not initialize video decoder");
            return;
        }
    }
    
    bool got_output;
    bool success = ffmpeg_decode_video(video_decoder, data, packet.size(), &ts,
                                       &frame, &got_output);
    if (!success)
    {
        blog(LOG_WARNING, "Error decoding video");
        return;
    }
    
    if (got_output && source != NULL)
    {
        frame.timestamp = (uint64_t)ts * 100;
        //if (flip)
        //frame.flip = !frame.flip;
#if LOG_ENCODED_VIDEO_TS
        blog(LOG_DEBUG, "video ts: %llu", frame.timestamp);
#endif
        obs_source_output_video(source, &frame);
    }
}

void *FFMpegVideoDecoder::run() {
    
    for (int i = 0;; i++) {
        
        PacketItem *item = (PacketItem *)mQueue.remove();
        this->processPacketItem(item);
        delete item;
    }
    
    return NULL;
}


