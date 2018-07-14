
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
    memset(&audio_frame, 0, sizeof(audio_frame));
}

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
    this->Shutdown();
    // Free the video decoder.
    ffmpeg_decode_free(video_decoder);
    ffmpeg_decode_free(audio_decoder);
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
    
    if (!ffmpeg_decode_valid(audio_decoder))
    {
        if (ffmpeg_decode_init(audio_decoder, AV_CODEC_ID_AAC) < 0)
        {
            blog(LOG_WARNING, "Could not initialize audio decoder");
            return;
        }
    }
    
    auto packet = packetItem->getPacket();
    unsigned char *data = (unsigned char *)packet.data();
    long long ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (packetItem->getType() == 102) {
        
        bool got_output;
        
        bool success = ffmpeg_decode_audio(audio_decoder, data, packet.size(), &audio_frame, &got_output);
        
        if (!success)
        {
            blog(LOG_WARNING, "Error decoding audio");
            return;
        }
        
        if (got_output && source != NULL)
        {
            audio_frame.timestamp = os_gettime_ns() - 100000000; // -100ms
            obs_source_output_audio(source, &audio_frame);
        }
        
    }
    else if (packetItem->getType() == 101) {
        
        bool got_output;
        bool success = ffmpeg_decode_video(video_decoder, data, packet.size(), &ts,
                                           &video_frame, &got_output);
        if (!success)
        {
            //        blog(LOG_WARNING, "Error decoding video");
            return;
        }
        
        if (got_output && source != NULL)
        {
//            frame.timestamp = (uint64_t)ts * 100;
            video_frame.timestamp = os_gettime_ns() - 100000000; // -100ms
            //if (flip)
            //frame.flip = !frame.flip;
#if LOG_ENCODED_VIDEO_TS
            blog(LOG_DEBUG, "video ts: %llu", video_frame.timestamp);
#endif
            obs_source_output_video(source, &video_frame);
        }
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
            
            if (queueSize > 25) {
                while (mQueue.size() > 5) {
                    mQueue.remove();
                }
            }
            
        }
        
    }
    
    return NULL;
}


