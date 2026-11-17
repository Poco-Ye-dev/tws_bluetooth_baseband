#include <audio/anc.h>
#include <core/string.h>
#include <core/math.h>

int anc_init(struct anc *anc, const struct anc_config *config)
{
    if (!anc || !config || !config->sample_rate)
        return -EINVAL;

    memset(anc, 0, sizeof(*anc));
    anc->config = *config;
    anc->mode = ANC_MODE_OFF;
    return 0;
}

int anc_set_mode(struct anc *anc, enum anc_mode mode)
{
    if (!anc || mode > ANC_MODE_TRANSPARENCY)
        return -EINVAL;

    anc->mode = mode;
    anc->enabled = mode == ANC_MODE_ACTIVE || mode == ANC_MODE_TRANSPARENCY;
    return 0;
}

enum anc_mode anc_get_mode(struct anc *anc)
{
    return anc ? anc->mode : ANC_MODE_OFF;
}

int anc_process(struct anc *anc, const struct audio_buffer *reference,
                const struct audio_buffer *playback, struct audio_buffer *out)
{
    if (!anc || !playback || !out)
        return -EINVAL;

    if (!anc->enabled || !reference) {
        *out = *playback;
        return 0;
    }

    const s16 *ref = reference->data;
    const s16 *play = playback->data;
    s16 *dst = out->data;
    u32 samples = playback->frames * playback->channels;
    s16 gain = anc->config.gain_q15;

    for (u32 i = 0; i < samples; i++)
        dst[i] = sat16((s32)play[i] - q15_mul(ref[i], gain));

    return 0;
}

void anc_deinit(struct anc *anc)
{
    if (!anc)
        return;
    anc->enabled = false;
    anc->mode = ANC_MODE_OFF;
}
