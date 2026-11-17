#ifndef __AUDIO_CVP_H__
#define __AUDIO_CVP_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/buffer.h>

/*
 * Clear voice pickup: two microphone beam forming plus echo cancellation for
 * the uplink path.
 */

struct cvp_config {
    u32 sample_rate;
    bool echo_cancel;
    bool noise_suppress;
    u8 mic_count;
    s16 mic_gain_q15;
};

struct cvp {
    struct cvp_config config;
    bool enabled;
    void *priv;
};

int cvp_init(struct cvp *cvp, const struct cvp_config *config);
int cvp_enable(struct cvp *cvp, bool enable);
int cvp_process(struct cvp *cvp, const struct audio_buffer *mic0,
                const struct audio_buffer *mic1, struct audio_buffer *out);
void cvp_deinit(struct cvp *cvp);

#endif /* __AUDIO_CVP_H__ */
