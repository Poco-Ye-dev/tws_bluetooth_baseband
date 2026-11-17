#ifndef __AUDIO_SMART_VOICE_H__
#define __AUDIO_SMART_VOICE_H__

#include <core/types.h>
#include <audio/dsp.h>

/*
 * Lightweight wake word detector.
 *
 * Instead of a neural model this extracts an 8 element spectral envelope from
 * each frame and compares it with trained templates using a scaled L1
 * distance.  It is small enough for parts without an NPU and deterministic,
 * which matters for latency measurements.
 */

#define SV_FEATURES 8
#define SV_MAX_TEMPLATES 4
#define SV_NAME_MAX 16

struct sv_template {
    char name[SV_NAME_MAX];
    u8 features[SV_FEATURES];
    bool used;
};

typedef void (*sv_callback_t)(const char *name, void *arg);

struct smart_voice {
    struct vad vad;
    struct sv_template templates[SV_MAX_TEMPLATES];
    u8 count;
    u8 threshold;
    sv_callback_t callback;
    void *arg;
};

int sv_init(struct smart_voice *sv, sv_callback_t cb, void *arg);
int sv_add_template(struct smart_voice *sv, const char *name,
                    const s16 *samples, u32 count);
int sv_process(struct smart_voice *sv, const s16 *samples, u32 count);
void sv_set_threshold(struct smart_voice *sv, u8 threshold);

#endif /* __AUDIO_SMART_VOICE_H__ */
