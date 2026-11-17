#include <media/codec.h>
#include <core/mem.h>
#include <core/string.h>

/*
 * SBC / mSBC codec front end.
 *
 * The bitstream transform is provided by the platform DSP; this layer owns
 * configuration, framing and buffer accounting so the audio path sees the
 * same interface as every other codec.
 */

#define SBC_DEFAULT_FRAME 128
#define MSBC_FRAME 57

struct sbc_ctx {
    struct codec_config config;
    enum codec_type type;
    bool encode;
    u8 frame[SBC_DEFAULT_FRAME];
};

static int sbc_open(void **ctx, const struct codec_config *cfg, bool encode)
{
    struct sbc_ctx *sbc;

    if (!ctx || !cfg || !cfg->sample_rate)
        return -EINVAL;

    sbc = kzalloc(sizeof(*sbc));
    if (!sbc)
        return -ENOMEM;

    sbc->config = *cfg;
    sbc->encode = encode;
    *ctx = sbc;
    return 0;
}

static int sbc_process(void *ctx, const struct audio_buffer *in,
                       struct audio_buffer *out)
{
    struct sbc_ctx *sbc = ctx;
    u32 bytes;

    if (!sbc || !in || !out)
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

static int sbc_close(void *ctx)
{
    kfree(ctx);
    return 0;
}

static u32 sbc_frame_size(void *ctx)
{
    struct sbc_ctx *sbc = ctx;
    return sbc && sbc->type == CODEC_TYPE_MSBC ? MSBC_FRAME : SBC_DEFAULT_FRAME;
}

static const struct codec_ops sbc_ops = {
    .name = "sbc",
    .type = CODEC_TYPE_SBC,
    .open = sbc_open,
    .process = sbc_process,
    .close = sbc_close,
    .frame_size = sbc_frame_size,
};

static const struct codec_ops msbc_ops = {
    .name = "msbc",
    .type = CODEC_TYPE_MSBC,
    .open = sbc_open,
    .process = sbc_process,
    .close = sbc_close,
    .frame_size = sbc_frame_size,
};

int codec_sbc_register(void)
{
    int ret = codec_register(&sbc_ops);
    return ret ? ret : codec_register(&msbc_ops);
}
