/*
 obs-ios-camera-source
 Copyright (C) 2018 Will Townsend <will@townsend.io>
 
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

class VideoDecoderCallback {
public:
    virtual ~VideoDecoderCallback() {}
};

class VideoDecoder {
protected:
    virtual ~VideoDecoder() {};
public:
    virtual void Init() = 0;
    virtual void Input(std::vector<char> packet, int type, int tag) = 0;
    virtual void Flush() = 0;
    virtual void Drain() = 0;
    virtual void Shutdown() = 0;
};

#endif /* VideoDecoderCallback_h */
