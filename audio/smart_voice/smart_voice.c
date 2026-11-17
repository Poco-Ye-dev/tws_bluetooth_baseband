#include <audio/smart_voice.h>
#include <core/string.h>
#include <core/math.h>

/*
 * Feature vector: energy in eight coarse bands approximated by successive
 * differences.  This is cheap (no FFT) and captures enough of the envelope
 * shape to separate a few fixed wake words.
 */

static void extract(const s16 *samples, u32 count, u8 features[SV_FEATURES])
{
    s32 diff[SV_FEATURES];
    s32 max = 1;

    memset(diff, 0, sizeof(diff));

    for (u32 i = 1; i < count; i++) {
        s32 d = abs32((s32)samples[i] - samples[i - 1]);
        u32 band = (u32)((u64)i * SV_FEATURES / count);
        if (band >= SV_FEATURES)
            band = SV_FEATURES - 1;
        diff[band] += d;
        if (d > max)
            max = d;
    }

    for (int i = 0; i < SV_FEATURES; i++)
        features[i] = (u8)((diff[i] * 255) / max > 255 ? 255 :
                           (diff[i] * 255) / max);
}

static u32 distance(const u8 *a, const u8 *b)
{
    u32 sum = 0;
    for (int i = 0; i < SV_FEATURES; i++)
        sum = (u32)(sum + abs32((s32)a[i] - b[i]));
    return sum;
}

int sv_init(struct smart_voice *sv, sv_callback_t cb, void *arg)
{
    if (!sv)
        return -1;

    memset(sv, 0, sizeof(*sv));
    sv->callback = cb;
    sv->arg = arg;
    sv->threshold = 120;
    vad_init(&sv->vad, 500, 5);
    return 0;
}

int sv_add_template(struct smart_voice *sv, const char *name,
                    const s16 *samples, u32 count)
{
    if (!sv || !name || !samples || !count || sv->count >= SV_MAX_TEMPLATES)
        return -1;

    struct sv_template *tpl = &sv->templates[sv->count];
    strncpy(tpl->name, name, SV_NAME_MAX - 1);
    extract(samples, count, tpl->features);
    tpl->used = true;
    sv->count++;
    return 0;
}

int sv_process(struct smart_voice *sv, const s16 *samples, u32 count)
{
    u8 features[SV_FEATURES];
    u32 best = 0xFFFFFFFFu;
    const struct sv_template *match = NULL;

    if (!sv || !samples || !count)
        return -1;
    if (!vad_process(&sv->vad, samples, count))
        return 0;

    extract(samples, count, features);

    for (unsigned int i = 0; i < sv->count; i++) {
        if (!sv->templates[i].used)
            continue;
        u32 d = distance(features, sv->templates[i].features);
        if (d < best) {
            best = d;
            match = &sv->templates[i];
        }
    }

    if (match && best <= sv->threshold) {
        if (sv->callback)
            sv->callback(match->name, sv->arg);
        return 1;
    }
    return 0;
}

void sv_set_threshold(struct smart_voice *sv, u8 threshold)
{
    if (sv)
        sv->threshold = threshold;
}
