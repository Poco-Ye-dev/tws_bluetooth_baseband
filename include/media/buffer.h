#ifndef __MEDIA_BUFFER_H__
#define __MEDIA_BUFFER_H__

#include <core/types.h>

/*
 * Audio frame buffer shared by codecs, effects and the DMA path.  Buffers are
 * described by a pointer plus frame count; ownership is explicit and always
 * returns to the pool that handed the buffer out.
 */

enum sample_format {
    SAMPLE_FMT_S16 = 0,
    SAMPLE_FMT_S24,
    SAMPLE_FMT_S32,
};

struct audio_buffer {
    void *data;
    u32 bytes;
    u32 frames;
    u8 channels;
    u8 bits;
    u32 sample_rate;
    u32 sequence;
    bool in_use;
};

void audio_buffer_reset(struct audio_buffer *buf);
u32 audio_buffer_frames(const struct audio_buffer *buf);
u32 audio_buffer_duration_ms(const struct audio_buffer *buf);

#endif /* __MEDIA_BUFFER_H__ */
