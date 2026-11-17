#ifndef __AUDIO_ANC_H__
#define __AUDIO_ANC_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/buffer.h>

/*
 * Active noise cancellation.  The algorithm runs on the same frame boundary
 * as the playback path, so the caller feeds reference (ambient) and playback
 * buffers and receives the anti-noise mix.
 */

enum anc_mode {
    ANC_MODE_OFF = 0,
    ANC_MODE_PASSIVE,
    ANC_MODE_ACTIVE,
    ANC_MODE_TRANSPARENCY,
};

struct anc_config {
    u32 sample_rate;
    u8 channels;
    u8 filter_length;
    s16 gain_q15;
};

struct anc {
    enum anc_mode mode;
    struct anc_config config;
    bool enabled;
    void *priv;
};

int anc_init(struct anc *anc, const struct anc_config *config);
int anc_set_mode(struct anc *anc, enum anc_mode mode);
enum anc_mode anc_get_mode(struct anc *anc);
int anc_process(struct anc *anc, const struct audio_buffer *reference,
                const struct audio_buffer *playback, struct audio_buffer *out);
void anc_deinit(struct anc *anc);

#endif /* __AUDIO_ANC_H__ */
