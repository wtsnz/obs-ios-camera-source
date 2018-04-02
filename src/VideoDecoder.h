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

#ifndef VideoDecoderCallback_h
#define VideoDecoderCallback_h

#include <obs.h>
#include <vector>


// A callback used by MediaDataDecoder to return output/errors to the
// MediaFormatReader.
// Implementation is threadsafe, and can be called on any thread.
class VideoDecoderCallback {
public:
    virtual ~VideoDecoderCallback() {}
    
//    virtual void Output
    
    // Called by MediaDataDecoder when a sample has been decoded.
//    virtual void Output(void OutputFrame(CVPixelBufferRef aImage, CMVideoFormatDescriptionRef formatDescription);) = 0;
    
    // Denotes an error in the decoding process. The reader will stop calling
    // the decoder.
    //    virtual void Error(const MediaResult& aError) = 0;
    
    // Denotes that the last input sample has been inserted into the decoder,
    // and no more output can be produced unless more input is sent.
    // A frame decoding session is completed once InputExhausted has been called.
    // MediaDataDecoder::Input will not be called again until InputExhausted has
    // been called.
//    virtual void InputExhausted() = 0;
//
//    virtual void DrainComplete() = 0;
//
//    virtual void ReleaseMediaResources() {}
//
//    virtual bool OnReaderTaskQueue() = 0;
};

class VideoDecoder {
protected:
    virtual ~VideoDecoder() {};
public:
    virtual void Init() = 0;
    virtual void Input(std::vector<char> packet) = 0;
    virtual void Flush() = 0;
    virtual void Drain() = 0;
    virtual void Shutdown() = 0;
//    virtual void OutputFrame(
};

#endif /* VideoDecoderCallback_h */
