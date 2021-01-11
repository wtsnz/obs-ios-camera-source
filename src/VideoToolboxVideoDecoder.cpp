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

#import "VideoToolboxVideoDecoder.h"

#define NAL_LENGTH_PREFIX_SIZE 4

VideoToolboxDecoder::VideoToolboxDecoder()
{
    waitingForSps = true;
    waitingForPps = true;
    mSession = NULL;
    mFormat = NULL;

    memset(&frame, 0, sizeof(frame));
}

VideoToolboxDecoder::~VideoToolboxDecoder()
{
    this->Shutdown();
}

void VideoToolboxDecoder::Init()
{
    // Start the thread.
    this->start();
}

void VideoToolboxDecoder::Flush()
{
    std::lock_guard<std::mutex> lock (mMutex);

    // Clear the queue
    while(this->mQueue.size() > 0) {
        this->mQueue.remove();
    }

    VTDecompressionSessionInvalidate(mSession);
    mSession = NULL;
}

void VideoToolboxDecoder::Drain()
{

}

void VideoToolboxDecoder::Shutdown()
{
    std::lock_guard<std::mutex> lock (mMutex);

    mQueue.stop();

    if (mSession != NULL) {
        VTDecompressionSessionInvalidate(mSession);
    }

    mSession = NULL;

    this->join();
}

void *VideoToolboxDecoder::run() {

    while (shouldStop() == false) {
        PacketItem *item = (PacketItem *)mQueue.remove();
        if (item != NULL) {
            this->processPacketItem(item);
            delete item;
        }

        // Check queue lengths

        const int queueSize = mQueue.size();
        if (queueSize > 5) {
            blog(LOG_WARNING, "Video Toolbox: decoding queue overloaded. %d frames behind. Please use a lower quality setting.", queueSize);

            while (mQueue.size() > 0) {
                PacketItem *item = (PacketItem *)mQueue.remove();
                if (item != NULL) {
                    blog(LOG_INFO, "Video Toolbox: dropping packet type=%d tag=%d size=%d", item->getType(), item->getTag(), item->size());
                    delete item;
                }
            }
        }
    }

    return NULL;
}

static const char *video_toolbox_decode_video_name = "obs_camera_video_toolbox_decode_video";
void VideoToolboxDecoder::processPacketItem(PacketItem *packetItem)
{
    auto packet = packetItem->getPacket();

    //    blog(LOG_INFO, "Input");

    OSStatus status = 0;
    uint32_t frameSize = packet.size();

    if (frameSize < 3) {
        return;
    }

    std::lock_guard<std::mutex> lock (mMutex);

    int naluType = (packet[4] & 0x1F);

    if (naluType == 7 || naluType == 8) {

        // NALU is the SPS Parameter
        if (naluType == 7) {

            spsData = std::vector<char>(packet.begin() + 4, packet.end() + (frameSize - 4));

            waitingForSps = false;
            waitingForPps = true;

        }

        // NALU is the PPS Parameter
        if (naluType == 8) {

            ppsData = std::vector<char>(packet.begin() + 4, packet.end() + (frameSize - 4));

            waitingForPps = false;
        }

        if (!waitingForPps && !waitingForSps) {

            const uint8_t * const parameterSetPointers[] = { (uint8_t *)spsData.data(), (uint8_t *)ppsData.data() };
            const size_t parameterSetSizes[] = { spsData.size(), ppsData.size() };

            status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                         2, /* count of parameter sets */
                                                                         parameterSetPointers,
                                                                         parameterSetSizes,
                                                                         NAL_LENGTH_PREFIX_SIZE,
                                                                         &mFormat);

            if (status != noErr) {
                blog(LOG_INFO, "Failed to create format description");
                mFormat = NULL;
            } else {

                if (mSession == NULL) {
                    this->createDecompressionSession();
                } else {

                    bool needNewDecompSession = (VTDecompressionSessionCanAcceptFormatDescription(mSession, mFormat) == false);
                    if(needNewDecompSession) {
                        blog(LOG_INFO, "Created Decompression session");
                        this->createDecompressionSession();
                    }
                }

            }

        } else {
            return;
        }

    }

    // Ensure that
    if (ppsData.size() < 1 || spsData.size() < 1) {
        return;
    }

    if (waitingForSps || waitingForPps) {
        return;
    }

    if (mFormat == NULL) {
        return;
    }

    // This decoder only supports these two frames
    if (naluType != 1 && naluType != 5) {
        return;
    }


    if (mSession == NULL) {
        this->createDecompressionSession();
    }

    // Create the sample data for the decoder

    CMBlockBufferRef blockBuffer = NULL;
    long blockLength = 0;

    // type 5 is an IDR frame NALU. The SPS and PPS NALUs should always be followed by an IDR (or IFrame) NALU, as far as I know
    if (naluType == 5) {

        blockLength = frameSize;

        // replace the start code header on this NALU with its size.
        // AVCC format requires that you do this.
        // htonl converts the unsigned int from host to network byte order
        uint32_t dataLength32 = htonl (blockLength - 4);
        memcpy(packet.data(), &dataLength32, sizeof (uint32_t));

        // create a block buffer from the IDR NALU
        status = CMBlockBufferCreateWithMemoryBlock(NULL, packet.data(),  // memoryBlock to hold buffered data
                                                    blockLength,  // block length of the mem block in bytes.
                                                    kCFAllocatorNull, NULL,
                                                    0, // offsetToData
                                                    blockLength,   // dataLength of relevant bytes, starting at offsetToData
                                                    0, &blockBuffer);

    }

    // NALU type 1 is non-IDR (or PFrame) picture
    if (naluType == 1) {
        // non-IDR frames do not have an offset due to SPS and PSS, so the approach
        // is similar to the IDR frames just without the offset
        blockLength = frameSize;

        // again, replace the start header with the size of the NALU
        uint32_t dataLength32 = htonl (blockLength - 4);
        memcpy (packet.data(), &dataLength32, sizeof (uint32_t));

        status = CMBlockBufferCreateWithMemoryBlock(NULL, packet.data(),  // memoryBlock to hold data. If NULL, block will be alloc when needed
                                                    blockLength,  // overall length of the mem block in bytes
                                                    kCFAllocatorNull, NULL,
                                                    0,     // offsetToData
                                                    blockLength,  // dataLength of relevant data bytes, starting at offsetToData
                                                    0, &blockBuffer);

    }

    // now create our sample buffer from the block buffer,
    if (status != noErr) {
        //        NSLog(@"Error creating block buffer: %@", @(status));
        return;
    }

    if (!blockBuffer) {
        return;
    }

    if (!mFormat) {
        return;
    }

    // here I'm not bothering with any timing specifics since in this case we displayed all frames immediately
    CMSampleBufferRef sampleBuffer = NULL;
    const size_t sampleSize = blockLength;
    status = CMSampleBufferCreate(kCFAllocatorDefault,
                                  blockBuffer,
                                  true,
                                  NULL,
                                  NULL,
                                  mFormat,
                                  1,
                                  0,
                                  NULL,
                                  1,
                                  &sampleSize,
                                  &sampleBuffer);

    if (sampleBuffer != NULL) {
        VTDecodeFrameFlags flags = 0;
        VTDecodeInfoFlags flagOut;

        profile_start(video_toolbox_decode_video_name);

        auto now = os_gettime_ns();

        VTDecompressionSessionDecodeFrame(mSession, sampleBuffer, flags,
                                          (void*)now, &flagOut);

        CFRelease(sampleBuffer);

        profile_end(video_toolbox_decode_video_name);
    }
}



void VideoToolboxDecoder::Input(std::vector<char> packet, int type, int tag)
{
    // Create a new packet item and enqueue it.
    PacketItem *item = new PacketItem(packet, type, tag);
    this->mQueue.add(item);
}

void VideoToolboxDecoder::OutputFrame(CVPixelBufferRef pixelBufferRef)
{
    CVImageBufferRef     image = pixelBufferRef;
    //        obs_source_frame *frame = frame;

    // CMTime target_pts =
    // CMSampleBufferGetOutputPresentationTimeStamp(sampleBuffer);
    // CMTime target_pts_nano = CMTimeConvertScale(target_pts, NANO_TIMESCALE,
    // kCMTimeRoundingMethod_Default);
    // frame->timestamp = target_pts_nano.value;

    if (!update_frame(source, &frame, image, mFormat)) {
        // Send blank video
        obs_source_output_video(source, nullptr);
        return;
    }

    obs_source_output_video(source, &frame);

    CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
}

static void
DecompressionSessionDecodeFrameCallback(void *decompressionOutputRefCon,
                                        void *sourceFrameRefCon,
                                        OSStatus status,
                                        VTDecodeInfoFlags infoFlags,
                                        CVImageBufferRef imageBuffer,
                                        CMTime presentationTimeStamp,
                                        CMTime presentationDuration)
{

    VideoToolboxDecoder* decoder = static_cast<VideoToolboxDecoder*>(decompressionOutputRefCon);

    if (status != noErr || !imageBuffer) {
        blog(LOG_INFO, "VideoToolbox decoder returned no image");
    } else if (infoFlags & kVTDecodeInfo_FrameDropped) {
        blog(LOG_INFO, "VideoToolbox dropped frame");
    }

    decoder->OutputFrame(imageBuffer);
}

void VideoToolboxDecoder::createDecompressionSession()
{
    //    if (mSession != NULL) {
    //        VTDecompressionSessionInvalidate(mSession);
    //    }

    mSession = NULL;

    VTDecompressionOutputCallbackRecord callBackRecord;
    callBackRecord.decompressionOutputCallback = DecompressionSessionDecodeFrameCallback;
    callBackRecord.decompressionOutputRefCon = this;

    // Destination Pixel Buffer Attributes
    CFMutableDictionaryRef destinationPixelBufferAttributes;
    destinationPixelBufferAttributes = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFNumberRef number;

    int val = kCVPixelFormatType_32BGRA;
    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &val);
    CFDictionarySetValue(destinationPixelBufferAttributes, kCVPixelBufferPixelFormatTypeKey, number);
    CFRelease(number);

    // Format Pixel Buffer Attributes
    CFMutableDictionaryRef videoDecoderSpecification;
    videoDecoderSpecification = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFDictionarySetValue(videoDecoderSpecification, kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder, kCFBooleanTrue);

    int err = VTDecompressionSessionCreate(NULL, mFormat, videoDecoderSpecification, destinationPixelBufferAttributes, &callBackRecord, &mSession);
    if(err != noErr) {
        blog(LOG_ERROR, "Failed creating Decompression session");
        return;
    }
}

bool VideoToolboxDecoder::update_frame(obs_source_t *capture, obs_source_frame *frame, CVImageBufferRef imageBufferRef, CMVideoFormatDescriptionRef formatDesc)
{
    // blog(LOG_INFO, "Update frame");
    if (!formatDesc) {
        // blog(LOG_INFO, "No format");

        return false;
    }

    //    FourCharCode    fourcc = CMFormatDescriptionGetMediaSubType(formatDesc);
    // video_format    format = format_from_subtype(fourcc);
    CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(formatDesc);

    frame->timestamp = os_gettime_ns();
    frame->width    = dims.width;
    frame->height   = dims.height;
    frame->format   = VIDEO_FORMAT_BGRA;
    //        frame->format   = VIDEO_FORMAT_YUY2;

    CVPixelBufferLockBaseAddress(imageBufferRef, kCVPixelBufferLock_ReadOnly);

    if (!CVPixelBufferIsPlanar(imageBufferRef)) {
        frame->linesize[0] = CVPixelBufferGetBytesPerRow(imageBufferRef);
        frame->data[0]     = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(imageBufferRef));
        return true;
    }

    size_t count = CVPixelBufferGetPlaneCount(imageBufferRef);
    for (size_t i = 0; i < count; i++) {
        frame->linesize[i] = CVPixelBufferGetBytesPerRowOfPlane(imageBufferRef, i);
        frame->data[i]     = static_cast<uint8_t*>(CVPixelBufferGetBaseAddressOfPlane(imageBufferRef, i));
    }
    return true;

}

