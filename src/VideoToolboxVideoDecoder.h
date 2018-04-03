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

#include <obs.h>
#include <chrono>
#include <vector>

#include <VideoToolbox/VideoToolbox.h>

#include "Queue.hpp"
#include "Thread.hpp"
#include "VideoDecoder.h"

//class VideoToolboxDecoderCallback {
//public:
//    virtual ~VideoToolboxDecoderCallback() {}
//
//    virtual void VideoToolboxDecodedFrame(CVPixelBufferRef aImage, CMVideoFormatDescriptionRef formatDescription) = 0;
//};

class VideoToolboxDecoder: public VideoDecoder, private Thread
{
public:
    VideoToolboxDecoder();
    
    void Init() override;
    
    void Input(std::vector<char> packet) override;
    
    void Flush() override;
    void Drain() override;
    void Shutdown() override;
    
    void OutputFrame(CVPixelBufferRef pixelBufferRef);
        
    bool update_frame(obs_source_t *capture, obs_source_frame *frame, CVImageBufferRef imageBufferRef, CMVideoFormatDescriptionRef formatDesc);
    
    // The OBS Source to update.
    obs_source_t *source;
    
    
private:
    
    void *run() override; // Thread
    void processPacketItem(PacketItem *packetItem);
    
    void createDecompressionSession();
    
    CMVideoFormatDescriptionRef mFormat;
    VTDecompressionSessionRef mSession;
    
    bool waitingForSps;
    bool waitingForPps;
    
    std::vector<char> spsData;
    std::vector<char> ppsData;
    
    WorkQueue<PacketItem *> mQueue;
    
    obs_source_frame frame;
};
