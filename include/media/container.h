#ifndef __MEDIA_CONTAINER_H__
#define __MEDIA_CONTAINER_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/codec.h>

/*
 * Container parsing.  Firmware only needs the stream description and the
 * offset of the first frame, so this is a header reader rather than a full
 * demuxer.
 */

enum container_format {
    CONTAINER_UNKNOWN = 0,
    CONTAINER_WAV,
    CONTAINER_MP3,
    CONTAINER_AAC,
    CONTAINER_OGG,
};

struct audio_stream_info {
    enum container_format container;
    enum codec_type codec;
    u32 sample_rate;
    u8 channels;
    u8 bits;
    u32 bitrate;
    u32 data_offset;
    u32 data_size;
};

int container_probe(const void *data, u32 len, struct audio_stream_info *info);
int container_parse_wav(const void *data, u32 len, struct audio_stream_info *info);
const char *container_name(enum container_format format);

#endif /* __MEDIA_CONTAINER_H__ */
