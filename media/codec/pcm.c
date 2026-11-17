#include <media/codec.h>
#include <core/mem.h>
#include <core/string.h>

/*
 * PCM "codec" used for prompts, line-in and as a fallback when a hardware
 * codec is unavailable.  It also documents the shape every real codec
 * registration follows.
 */

struct pcm_ctx {
    struct codec_config config;
    bool encode;
};

static int pcm_open(void **ctx, const struct codec_config *cfg, bool encode)
{
    struct pcm_ctx *pcm;

    if (!ctx || !cfg || !cfg->sample_rate || !cfg->channels)
        return -EINVAL;

    pcm = kzalloc(sizeof(*pcm));
    if (!pcm)
        return -ENOMEM;

    pcm->config = *cfg;
    pcm->encode = encode;
    *ctx = pcm;
    return 0;
}

static int pcm_process(void *ctx, const struct audio_buffer *in,
                       struct audio_buffer *out)
{
    struct pcm_ctx *pcm = ctx;
    u32 bytes;

    if (!pcm || !in || !out)
        return -EINVAL;

    bytes = in->bytes;
    if (bytes > out->bytes)
        bytes = out->bytes;

    if (out->data && in->data)
        memcpy(out->data, in->data, bytes);

    out->bytes = bytes;
    out->frames = in->frames;
    out->channels = in->channels;
    out->bits = in->bits;
    out->sample_rate = in->sample_rate;
    return 0;
}

static int pcm_close(void *ctx)
{
    kfree(ctx);
    return 0;
}

static u32 pcm_frame_size(void *ctx)
{
    struct pcm_ctx *pcm = ctx;
    return pcm ? (u32)pcm->config.channels * (pcm->config.bits / 8) : 0;
}

static const struct codec_ops pcm_ops = {
    .name = "pcm",
    .type = CODEC_TYPE_PCM,
    .open = pcm_open,
    .process = pcm_process,
    .close = pcm_close,
    .frame_size = pcm_frame_size,
};

int codec_pcm_register(void)
{
    return codec_register(&pcm_ops);
}
