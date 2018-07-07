
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
    mQueue.stop();
    this->join();
}

void FFMpegVideoDecoder::Input(std::vector<char> packet, int type, int tag)
{
    // Create a new packet item and enqueue it.
    PacketItem *item = new PacketItem(packet, type, tag);
    this->mQueue.add(item);
    
    const int queueSize = mQueue.size();
    blog(LOG_WARNING, "Decoding queue size. %d frames behind.", queueSize);
}

void FFMpegVideoDecoder::processPacketItem(PacketItem *packetItem)
{
    
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
//        blog(LOG_WARNING, "Error decoding video");
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
    
    while (shouldStop() == false) {
        
        PacketItem *item = (PacketItem *)mQueue.remove();
        
        if (item != NULL) {
            this->processPacketItem(item);
            delete item;
        }
        
        // Check queue lengths
        
        const int queueSize = mQueue.size();
        if (queueSize > 25) {
            blog(LOG_WARNING, "Decoding queue overloaded. %d frames behind. Please use a lower quality setting.", queueSize);
        }
        
    }
    
    return NULL;
}


