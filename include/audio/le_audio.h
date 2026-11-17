#ifndef __AUDIO_LE_AUDIO_H__
#define __AUDIO_LE_AUDIO_H__

#include <core/types.h>

/*
 * LE Audio (LC3) presentation layer.
 *
 * The link layer (BIS/CIS) is owned by the Bluetooth stack; this module adds
 * LC3 framing, presentation delay and the encode/decode hand-off so the
 * product sees a normal audio stream.
 */

#define LE_AUDIO_FRAME_MS 10
#define LE_AUDIO_MAX_CHANNELS 2

enum le_audio_role {
    LE_AUDIO_SINK = 0,
    LE_AUDIO_SOURCE,
};

struct le_audio_config {
    enum le_audio_role role;
    u32 sample_rate;
    u8 channels;
    u8 frame_ms;
    u16 presentation_delay_us;
    u16 transport_latency_ms;
};

struct le_audio_stats {
    u32 frames_sent;
    u32 frames_received;
    u32 frames_lost;
    u32 late_frames;
};

struct le_audio {
    struct le_audio_config config;
    bool running;
    u32 sequence;
    struct le_audio_stats stats;
};

int le_audio_init(struct le_audio *le, const struct le_audio_config *config);
int le_audio_start(struct le_audio *le);
int le_audio_stop(struct le_audio *le);
int le_audio_encode(struct le_audio *le, const s16 *pcm, u32 frames,
                    u8 *out, u32 out_cap);
int le_audio_decode(struct le_audio *le, const u8 *frame, u32 len,
                    s16 *pcm, u32 pcm_cap);
const struct le_audio_stats *le_audio_stats(const struct le_audio *le);

#endif /* __AUDIO_LE_AUDIO_H__ */
