#include <audio/effect.h>
#include <audio/dsp.h>
#include <core/mem.h>
#include <core/string.h>

/*
 * Parametric equaliser built from cascaded peaking biquads.  Band count and
 * parameters come from the product profile; the coefficient design happens
 * once at open time so the audio callback only runs multiply/add.
 */

struct eq_ctx {
    struct biquad band[EQ_MAX_BANDS];
    u8 bands;
    u8 channels;
};

static int eq_open(void **ctx, const void *config)
{
    const struct eq_config *cfg = config;
    struct eq_ctx *eq;

    eq = kzalloc(sizeof(*eq));
    if (!eq)
        return -ENOMEM;

    if (cfg) {
        eq->bands = cfg->bands > EQ_MAX_BANDS ? EQ_MAX_BANDS : cfg->bands;
        eq->channels = cfg->channels ? cfg->channels : 1;
        for (u8 i = 0; i < eq->bands; i++) {
            biquad_design(&eq->band[i], BIQUAD_PEAKING,
                          cfg->sample_rate, cfg->band[i].frequency,
                          cfg->band[i].gain_db, cfg->band[i].q10);
        }
    }

    *ctx = eq;
    return 0;
}

static int eq_process(void *ctx, const struct audio_buffer *in,
                      struct audio_buffer *out)
{
    struct eq_ctx *eq = ctx;
    u32 samples;

    if (!eq || !in || !out)
        return -EINVAL;

    samples = in->bytes / 2;
    if (in->data != out->data && in->data && out->data)
        memcpy(out->data, in->data, in->bytes);
    else if (!in->data || !out->data)
        return -EINVAL;

    s16 *frames = out->data;
    for (u32 i = 0; i < samples; i++) {
        s16 value = frames[i];
        for (u8 b = 0; b < eq->bands; b++)
            value = biquad_process(&eq->band[b], value);
        frames[i] = value;
    }

    out->bytes = in->bytes;
    out->frames = in->frames;
    out->channels = in->channels;
    out->bits = in->bits;
    out->sample_rate = in->sample_rate;
    return 0;
}

static int eq_close(void *ctx)
{
    kfree(ctx);
    return 0;
}

static void eq_set_param(void *ctx, u32 id, s32 value)
{
    struct eq_ctx *eq = ctx;
    u8 band = (u8)(id >> 16);
    u16 param = (u16)id;

    if (!eq || band >= eq->bands)
        return;

    if (param == 0)
        eq->band[band].b0 = value;
    else if (param == 1)
        eq->band[band].b1 = value;
    else if (param == 2)
        eq->band[band].b2 = value;
    else if (param == 3)
        eq->band[band].a1 = value;
    else if (param == 4)
        eq->band[band].a2 = value;
}

static const struct effect_ops eq_ops = {
    .name = "eq",
    .open = eq_open,
    .process = eq_process,
    .close = eq_close,
    .set_param = eq_set_param,
};

int audio_eq_register(void)
{
    return effect_register(&eq_ops);
}
