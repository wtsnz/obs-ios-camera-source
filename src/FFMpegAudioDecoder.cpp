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

#include "FFMpegAudioDecoder.h"
#include <util/platform.h>
#include <fstream>

FFMpegAudioDecoder::FFMpegAudioDecoder()
{
    memset(&audio_frame, 0, sizeof(audio_frame));
}

FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
    this->Shutdown();
    // Free the video decoder.
    ffmpeg_decode_free(audio_decoder);
}

void FFMpegAudioDecoder::Init()
{
    // Start the thread.
    this->start();
}

void FFMpegAudioDecoder::Flush()
{
    // Clear the queue
}

void FFMpegAudioDecoder::Drain()
{
    // Drain the queue
}

void FFMpegAudioDecoder::Shutdown()
{
    mQueue.stop();
    this->join();
}

void FFMpegAudioDecoder::Input(std::vector<char> packet, int type, int tag)
{
    // Create a new packet item and enqueue it.
    PacketItem *item = new PacketItem(packet, type, tag);
    this->mQueue.add(item);
}

void FFMpegAudioDecoder::processPacketItem(PacketItem *packetItem)
{
    uint64_t cur_time = os_gettime_ns();

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
            audio_frame.timestamp = cur_time;
            obs_source_output_audio(source, &audio_frame);
        }
    }
}

void *FFMpegAudioDecoder::run() {

    while (shouldStop() == false) {

        PacketItem *item = (PacketItem *)mQueue.remove();

        if (item != NULL) {
            this->processPacketItem(item);
            delete item;
        }

        // Check queue lengths

        const int queueSize = mQueue.size();
        if (queueSize > 25) {
            blog(LOG_WARNING, "Audio Decoding queue overloaded. %d frames behind. Please use a lower quality setting.", queueSize);

            if (queueSize > 25) {
                while (mQueue.size() > 5) {
                    PacketItem *item = (PacketItem *)mQueue.remove();
                    if (item != NULL) {
                        delete item;
                    }
                }
            }
        }
    }
    return NULL;
}
