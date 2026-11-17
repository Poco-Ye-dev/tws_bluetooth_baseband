#include <media/codec.h>
#include <core/string.h>
#include <core/printk.h>

/*
 * Flat codec registry.  Registration happens at link time from each codec
 * module's init routine, which keeps the core free of codec knowledge.
 */

#define MAX_CODECS 16

static const struct codec_ops *codecs[MAX_CODECS];
static unsigned int codec_count;

int codec_register(const struct codec_ops *ops)
{
    if (!ops || !ops->open || !ops->process || codec_count >= MAX_CODECS)
        return -ENOSPC;

    codecs[codec_count++] = ops;
    pr_info("codec: registered %s\n", ops->name);
    return 0;
}

const struct codec_ops *codec_find(enum codec_type type, bool encode)
{
    for (unsigned int i = 0; i < codec_count; i++) {
        if (codecs[i]->type == type)
            return codecs[i];
    }
    (void)encode;
    return NULL;
}

const struct codec_ops *codec_find_name(const char *name, bool encode)
{
    for (unsigned int i = 0; i < codec_count; i++) {
        if (strcmp(codecs[i]->name, name) == 0)
            return codecs[i];
    }
    (void)encode;
    return NULL;
}

void audio_buffer_reset(struct audio_buffer *buf)
{
    if (!buf)
        return;

    buf->data = NULL;
    buf->bytes = 0;
    buf->frames = 0;
    buf->channels = 0;
    buf->bits = 0;
    buf->sample_rate = 0;
    buf->sequence = 0;
    buf->in_use = false;
}

u32 audio_buffer_frames(const struct audio_buffer *buf)
{
    if (!buf || !buf->bits || !buf->channels)
        return 0;

    u32 frame_bytes = (u32)buf->channels * (buf->bits / 8);
    return frame_bytes ? buf->bytes / frame_bytes : 0;
}

u32 audio_buffer_duration_ms(const struct audio_buffer *buf)
{
    if (!buf || !buf->sample_rate)
        return 0;
    return (audio_buffer_frames(buf) * 1000u) / buf->sample_rate;
}
