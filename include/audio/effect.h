#ifndef __AUDIO_EFFECT_H__
#define __AUDIO_EFFECT_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/buffer.h>

/*
 * Audio effects are pipeline stages with a stable name, so a product profile
 * can enable a chain by string instead of linking against concrete types.
 */

#define EFFECT_NAME_MAX 16
#define EQ_MAX_BANDS 10

struct effect_ops {
    const char *name;
    int (*open)(void **ctx, const void *config);
    int (*process)(void *ctx, const struct audio_buffer *in,
                   struct audio_buffer *out);
    int (*close)(void *ctx);
    void (*set_param)(void *ctx, u32 id, s32 value);
};

struct effect {
    const struct effect_ops *ops;
    void *ctx;
    bool enabled;
};

struct eq_band {
    u32 frequency;
    s16 gain_db;
    u16 q10;
};

struct eq_config {
    u32 sample_rate;
    u8 channels;
    u8 bands;
    struct eq_band band[EQ_MAX_BANDS];
};

int effect_register(const struct effect_ops *ops);
const struct effect_ops *effect_find(const char *name);

int effect_open(struct effect *effect, const char *name, const void *config);
int effect_process(struct effect *effect, const struct audio_buffer *in,
                   struct audio_buffer *out);
void effect_enable(struct effect *effect, bool enable);
void effect_set_param(struct effect *effect, u32 id, s32 value);
void effect_close(struct effect *effect);

#endif /* __AUDIO_EFFECT_H__ */
