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

#pragma once

#include <chrono>

#include "VideoDecoder.h"
#include "ffmpeg-decode.h"
#include "Queue.hpp"
#include "Thread.hpp"

class Decoder {
	struct ffmpeg_decode decode;

public:
	inline Decoder() { memset(&decode, 0, sizeof(decode)); }
	inline ~Decoder() { ffmpeg_decode_free(&decode); }

	inline operator ffmpeg_decode *() { return &decode; }
	inline ffmpeg_decode *operator->() { return &decode; }
};


class FFMpegVideoDecoder
	: public VideoDecoder,
	  private Thread,
	  public std::enable_shared_from_this<FFMpegVideoDecoder> {
public:
	class Delegate {
	public:
		virtual void ffmpegDecoderDidDecodeFrame(void *data, int stride,
							 int width,
							 int height) = 0;
		virtual ~Delegate(){};
	};

	FFMpegVideoDecoder();
	~FFMpegVideoDecoder();

	void Init() override;

	void Input(std::vector<char> packet, int type, int tag) override;

	void Flush() override;
	void Drain() override;
	void Shutdown() override;

	void setDelegate(std::shared_ptr<Delegate> newDelegate)
	{
		delegate = newDelegate;
	}

	std::weak_ptr<Delegate> getDelegate() { return delegate; };

	    obs_source_t *source;
	obs_source_frame video_frame;


private:
	void *run() override;

	void processPacketItem(PacketItem *packetItem);

	WorkQueue<PacketItem *> mQueue;

	Decoder video_decoder;
	std::weak_ptr<Delegate> delegate;
	std::mutex mMutex;

};

//
//class FFMpegVideoDecoderCallback {
//public:
//    virtual ~FFMpegVideoDecoderCallback() {}
//};
//
//class FFMpegVideoDecoder: public VideoDecoder, private Thread
//{
//public:
//    FFMpegVideoDecoder();
//    ~FFMpegVideoDecoder();
//    
//    void Init() override;
//    
//    void Input(std::vector<char> packet, int type, int tag) override;
//    
//    void Flush() override;
//    void Drain() override;
//    void Shutdown() override;
//    
//    obs_source_t *source;
//
//private:
//    
//    void *run() override;
//    
//    void processPacketItem(PacketItem *packetItem);
//    
//    WorkQueue<PacketItem *> mQueue;
//    
//    obs_source_frame video_frame;
//    
//    Decoder video_decoder;
//
//    std::mutex mMutex;
//};
