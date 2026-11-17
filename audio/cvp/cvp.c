#include <audio/cvp.h>
#include <core/string.h>
#include <core/math.h>

int cvp_init(struct cvp *cvp, const struct cvp_config *config)
{
    if (!cvp || !config || !config->sample_rate)
        return -EINVAL;

    memset(cvp, 0, sizeof(*cvp));
    cvp->config = *config;
    if (cvp->config.mic_count == 0)
        cvp->config.mic_count = 1;
    return 0;
}

int cvp_enable(struct cvp *cvp, bool enable)
{
    if (!cvp)
        return -EINVAL;
    cvp->enabled = enable;
    return 0;
}

int cvp_process(struct cvp *cvp, const struct audio_buffer *mic0,
                const struct audio_buffer *mic1, struct audio_buffer *out)
{
    if (!cvp || !mic0 || !out)
        return -EINVAL;

    if (!cvp->enabled || cvp->config.mic_count < 2 || !mic1) {
        *out = *mic0;
        return 0;
    }

    const s16 *a = mic0->data;
    const s16 *b = mic1->data;
    s16 *dst = out->data;
    u32 samples = mic0->frames * mic0->channels;

    for (u32 i = 0; i < samples; i++)
        dst[i] = sat16(((s32)a[i] + b[i]) / 2);

    return 0;
}

void cvp_deinit(struct cvp *cvp)
{
    if (cvp)
        cvp->enabled = false;
}
