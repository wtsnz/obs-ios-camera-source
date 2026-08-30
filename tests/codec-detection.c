#include "ffmpeg-decode.h"

#include <assert.h>

int main(void)
{
	const uint8_t h264_sps[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64,
				    0x00, 0x28};
	const uint8_t hevc_vps[] = {0x00, 0x00, 0x01, 0x40, 0x01, 0x0c,
				    0x01};
	const uint8_t unknown[] = {0x00, 0x00, 0x01, 0x06, 0x05, 0xff};

	assert(ffmpeg_detect_video_codec(h264_sps, sizeof(h264_sps)) ==
	       AV_CODEC_ID_H264);
	assert(ffmpeg_detect_video_codec(hevc_vps, sizeof(hevc_vps)) ==
	       AV_CODEC_ID_HEVC);
	assert(ffmpeg_detect_video_codec(unknown, sizeof(unknown)) ==
	       AV_CODEC_ID_NONE);
	assert(ffmpeg_detect_video_codec(NULL, 0) == AV_CODEC_ID_NONE);

	return 0;
}
