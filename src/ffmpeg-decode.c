/******************************************************************************
 Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "ffmpeg-decode.h"
#ifdef __has_include
#if __has_include("obs-ffmpeg-compat.h")
#include "obs-ffmpeg-compat.h"
#endif
#else
#include "obs-ffmpeg-compat.h"
#endif
#include <obs-avc.h>

static enum AVPixelFormat select_hardware_format(AVCodecContext *context,
                                                  const enum AVPixelFormat *formats)
{
    struct ffmpeg_decode *decode = context->opaque;
    for (const enum AVPixelFormat *format = formats;
         *format != AV_PIX_FMT_NONE; format++)
    {
        if (*format == decode->hardware_pixel_format)
            return *format;
    }

    return formats[0];
}

static int ffmpeg_decode_init_internal(struct ffmpeg_decode *decode,
                                       enum AVCodecID id,
                                       enum AVHWDeviceType hardware_type)
{
    int ret;

    memset(decode, 0, sizeof(*decode));

    decode->codec = avcodec_find_decoder(id);
    if (!decode->codec)
        return -1;

    decode->decoder = avcodec_alloc_context3(decode->codec);
    if (!decode->decoder)
        return AVERROR(ENOMEM);

    if (hardware_type != AV_HWDEVICE_TYPE_NONE)
    {
        const AVCodecHWConfig *configuration = NULL;
        for (int index = 0;
             (configuration = avcodec_get_hw_config(decode->codec, index));
             index++)
        {
            if ((configuration->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                configuration->device_type == hardware_type)
            {
                decode->hardware_pixel_format = configuration->pix_fmt;
                break;
            }
        }

        if (configuration &&
            av_hwdevice_ctx_create(&decode->hardware_device, hardware_type,
                                   NULL, NULL, 0) >= 0)
        {
            decode->hardware_device_type = hardware_type;
            decode->hardware_active = true;
            decode->decoder->opaque = decode;
            decode->decoder->get_format = select_hardware_format;
            decode->decoder->hw_device_ctx =
                av_buffer_ref(decode->hardware_device);
        }
    }

    ret = avcodec_open2(decode->decoder, decode->codec, NULL);
    if (ret < 0)
    {
        ffmpeg_decode_free(decode);
        return ret;
    }

#if LIBAVCODEC_VERSION_MAJOR < 60
    if (decode->codec->capabilities & AV_CODEC_CAP_TRUNCATED)
        decode->decoder->flags |= AV_CODEC_FLAG_TRUNCATED;
#endif

    decode->decoder->flags |= AV_CODEC_FLAG_LOW_DELAY;
    decode->decoder->flags2 = AV_CODEC_FLAG2_CHUNKS;

    return 0;
}

int ffmpeg_decode_init(struct ffmpeg_decode *decode, enum AVCodecID id)
{
    return ffmpeg_decode_init_internal(decode, id, AV_HWDEVICE_TYPE_NONE);
}

int ffmpeg_decode_init_hardware(struct ffmpeg_decode *decode,
                                enum AVCodecID id,
                                enum AVHWDeviceType type)
{
    return ffmpeg_decode_init_internal(decode, id, type);
}

void ffmpeg_decode_free(struct ffmpeg_decode *decode)
{
    if (decode->decoder)
    {
        avcodec_free_context(&decode->decoder);
    }

    if (decode->frame)
        av_frame_free(&decode->frame);

    if (decode->software_frame)
        av_frame_free(&decode->software_frame);

    if (decode->hardware_device)
        av_buffer_unref(&decode->hardware_device);

    if (decode->packet_buffer)
        bfree(decode->packet_buffer);

    memset(decode, 0, sizeof(*decode));
}

void ffmpeg_decode_flush(struct ffmpeg_decode *decode)
{
    if (decode->decoder)
        avcodec_flush_buffers(decode->decoder);
}

enum AVCodecID ffmpeg_detect_video_codec(const uint8_t *data, size_t size)
{
    if (!data || size < 5)
        return AV_CODEC_ID_NONE;

    for (size_t i = 0; i + 4 < size; i++)
    {
        size_t header = 0;
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
            header = i + 3;
        else if (i + 5 < size && data[i] == 0 && data[i + 1] == 0 &&
                 data[i + 2] == 0 && data[i + 3] == 1)
            header = i + 4;
        else
            continue;

        const uint8_t h264_type = data[header] & 0x1f;
        const uint8_t hevc_type = (data[header] >> 1) & 0x3f;

        /* Parameter-set NAL units identify the codec without guessing from
         * slice data, whose type values can overlap between H.264 and HEVC. */
        if (hevc_type >= 32 && hevc_type <= 34)
            return AV_CODEC_ID_HEVC;
        if (h264_type == 7 || h264_type == 8)
            return AV_CODEC_ID_H264;
    }

    return AV_CODEC_ID_NONE;
}

static inline enum video_format convert_pixel_format(int f)
{
    switch (f)
    {
        case AV_PIX_FMT_NONE:
            return VIDEO_FORMAT_NONE;
        case AV_PIX_FMT_YUV420P:
            return VIDEO_FORMAT_I420;
        case AV_PIX_FMT_NV12:
            return VIDEO_FORMAT_NV12;
        case AV_PIX_FMT_YUYV422:
            return VIDEO_FORMAT_YUY2;
        case AV_PIX_FMT_UYVY422:
            return VIDEO_FORMAT_UYVY;
        case AV_PIX_FMT_RGBA:
            return VIDEO_FORMAT_RGBA;
        case AV_PIX_FMT_BGRA:
            return VIDEO_FORMAT_BGRA;
        case AV_PIX_FMT_BGR0:
            return VIDEO_FORMAT_BGRX;
        case AV_PIX_FMT_YUVJ420P:
            return VIDEO_FORMAT_I420;
        default:;
    }

    return VIDEO_FORMAT_NONE;
}

static inline enum audio_format convert_sample_format(int f)
{
    switch (f)
    {
        case AV_SAMPLE_FMT_U8:
            return AUDIO_FORMAT_U8BIT;
        case AV_SAMPLE_FMT_S16:
            return AUDIO_FORMAT_16BIT;
        case AV_SAMPLE_FMT_S32:
            return AUDIO_FORMAT_32BIT;
        case AV_SAMPLE_FMT_FLT:
            return AUDIO_FORMAT_FLOAT;
        case AV_SAMPLE_FMT_U8P:
            return AUDIO_FORMAT_U8BIT_PLANAR;
        case AV_SAMPLE_FMT_S16P:
            return AUDIO_FORMAT_16BIT_PLANAR;
        case AV_SAMPLE_FMT_S32P:
            return AUDIO_FORMAT_32BIT_PLANAR;
        case AV_SAMPLE_FMT_FLTP:
            return AUDIO_FORMAT_FLOAT_PLANAR;
        default:;
    }

    return AUDIO_FORMAT_UNKNOWN;
}

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
    switch (channels)
    {
        case 0:
            return SPEAKERS_UNKNOWN;
        case 1:
            return SPEAKERS_MONO;
        case 2:
            return SPEAKERS_STEREO;
        case 3:
            return SPEAKERS_2POINT1;
        case 4:
            return SPEAKERS_4POINT0;
        case 5:
            return SPEAKERS_4POINT1;
        case 6:
            return SPEAKERS_5POINT1;
        case 8:
            return SPEAKERS_7POINT1;
        default:
            return SPEAKERS_UNKNOWN;
    }
}

static inline void copy_data(struct ffmpeg_decode *decode, uint8_t *data,
                             size_t size)
{
    size_t new_size = size + AV_INPUT_BUFFER_PADDING_SIZE;

    if (decode->packet_size < new_size)
    {
        decode->packet_buffer = brealloc(decode->packet_buffer,
                                         new_size);
        decode->packet_size = new_size;
    }

    memset(decode->packet_buffer + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    if (size)
        memcpy(decode->packet_buffer, data, size);
}

bool ffmpeg_decode_audio(struct ffmpeg_decode *decode,
                         uint8_t *data, size_t size,
                         struct obs_source_audio *audio,
                         bool *got_output)
{
    AVPacket packet = {0};
    int got_frame = false;
    int ret = 0;

    *got_output = false;

    copy_data(decode, data, size);

#if LIBAVCODEC_VERSION_MAJOR < 60
    av_init_packet(&packet);
#endif
    packet.data = decode->packet_buffer;
    packet.size = (int)size;

    if (!decode->frame)
    {
        decode->frame = av_frame_alloc();
        if (!decode->frame)
            return false;
    }

    if (data && size)
        ret = avcodec_send_packet(decode->decoder, &packet);
    if (ret == 0)
        ret = avcodec_receive_frame(decode->decoder, decode->frame);

    got_frame = (ret == 0);

    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        ret = 0;

    if (ret < 0)
        return false;
    else if (!got_frame)
        return true;

    for (size_t i = 0; i < MAX_AV_PLANES; i++)
        audio->data[i] = decode->frame->data[i];

    audio->samples_per_sec = decode->frame->sample_rate;
    audio->format = convert_sample_format(decode->frame->format);
    audio->speakers =
#if LIBAVUTIL_VERSION_MAJOR >= 57
    convert_speaker_layout((uint8_t)decode->frame->ch_layout.nb_channels);
#else
    convert_speaker_layout((uint8_t)decode->decoder->channels);
#endif

    audio->frames = decode->frame->nb_samples;

    if (audio->format == AUDIO_FORMAT_UNKNOWN)
        return false;

    *got_output = true;
    return true;
}

bool ffmpeg_decode_video(struct ffmpeg_decode *decode,
                         uint8_t *data, size_t size, long long *ts,
                         struct obs_source_frame *frame,
                         bool *got_output)
{
    AVPacket packet = {0};
    int got_frame = false;
    enum video_format new_format;
    AVFrame *decoded_frame;
    int ret;

    *got_output = false;

    copy_data(decode, data, size);

#if LIBAVCODEC_VERSION_MAJOR < 60
    av_init_packet(&packet);
#endif
    packet.data = decode->packet_buffer;
    packet.size = (int)size;
    packet.pts = *ts;

    if (data && size && decode->codec->id == AV_CODEC_ID_H264 &&
        obs_avc_keyframe(data, size))
    {
        packet.flags |= AV_PKT_FLAG_KEY;
    }

    if (!decode->frame)
    {
        decode->frame = av_frame_alloc();
        if (!decode->frame)
            return false;
    }

    if (data && size)
        ret = avcodec_send_packet(decode->decoder, &packet);
    else
        ret = 0;
    if (ret == 0)
        ret = avcodec_receive_frame(decode->decoder, decode->frame);

    got_frame = (ret == 0);

    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
        ret = 0;

    if (ret < 0)
        return false;
    else if (!got_frame)
        return true;

    decoded_frame = decode->frame;
    if (decode->hardware_active &&
        decode->frame->format == decode->hardware_pixel_format)
    {
        if (!decode->software_frame)
        {
            decode->software_frame = av_frame_alloc();
            if (!decode->software_frame)
                return false;
        }

        av_frame_unref(decode->software_frame);
        ret = av_hwframe_transfer_data(decode->software_frame,
                                       decode->frame, 0);
        if (ret < 0)
            return false;

        ret = av_frame_copy_props(decode->software_frame, decode->frame);
        if (ret < 0)
            return false;

        decoded_frame = decode->software_frame;
    }

    for (size_t i = 0; i < MAX_AV_PLANES; i++)
    {
        frame->data[i] = decoded_frame->data[i];
        frame->linesize[i] = decoded_frame->linesize[i];
    }

    new_format = convert_pixel_format(decoded_frame->format);
    if (new_format != frame->format)
    {
        bool success;
        enum video_range_type range;

        frame->format = new_format;
        frame->full_range =
        decoded_frame->color_range == AVCOL_RANGE_JPEG;

        range = frame->full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;

        success = video_format_get_parameters(VIDEO_CS_709,
                                              range, frame->color_matrix,
                                              frame->color_range_min, frame->color_range_max);
        if (!success)
        {
            blog(LOG_ERROR, "Failed to get video format "
                 "parameters for video format %u",
                 VIDEO_CS_709);
            return false;
        }
    }

    *ts = decoded_frame->pts;

    frame->width = decoded_frame->width;
    frame->height = decoded_frame->height;
    frame->flip = false;

    if (frame->format == VIDEO_FORMAT_NONE)
        return false;

    *got_output = true;
    return true;
}
