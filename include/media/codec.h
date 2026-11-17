#ifndef __MEDIA_CODEC_H__
#define __MEDIA_CODEC_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/buffer.h>

/*
 * Codec registry.  Decoders and encoders are described by the same ops table;
 * the direction is implied by which entry point is used.
 */

enum codec_type {
    CODEC_TYPE_SBC = 0,
    CODEC_TYPE_MSBC,
    CODEC_TYPE_AAC,
    CODEC_TYPE_MP3,
    CODEC_TYPE_PCM,
    CODEC_TYPE_OPUS,
    CODEC_TYPE_COUNT,
};

struct codec_config {
    u32 sample_rate;
    u8 channels;
    u8 bits;
    u32 bitrate;
    u32 frame_size;
};

struct codec_ops {
    const char *name;
    enum codec_type type;
    int (*open)(void **ctx, const struct codec_config *cfg, bool encode);
    int (*process)(void *ctx, const struct audio_buffer *in,
                   struct audio_buffer *out);
    int (*close)(void *ctx);
    u32 (*frame_size)(void *ctx);
};

int codec_register(const struct codec_ops *ops);
const struct codec_ops *codec_find(enum codec_type type, bool encode);
const struct codec_ops *codec_find_name(const char *name, bool encode);

#endif /* __MEDIA_CODEC_H__ */
