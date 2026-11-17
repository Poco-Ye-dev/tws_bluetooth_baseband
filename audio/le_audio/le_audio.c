#include <audio/le_audio.h>
#include <core/string.h>

int le_audio_init(struct le_audio *le, const struct le_audio_config *config)
{
    if (!le || !config || !config->channels || !config->sample_rate)
        return -1;

    memset(le, 0, sizeof(*le));
    le->config = *config;
    if (!le->config.frame_ms)
        le->config.frame_ms = LE_AUDIO_FRAME_MS;
    return 0;
}

int le_audio_start(struct le_audio *le)
{
    if (!le)
        return -1;
    le->running = true;
    le->sequence = 0;
    return 0;
}

int le_audio_stop(struct le_audio *le)
{
    if (!le)
        return -1;
    le->running = false;
    return 0;
}

int le_audio_encode(struct le_audio *le, const s16 *pcm, u32 frames,
                    u8 *out, u32 out_cap)
{
    u32 samples;

    if (!le || !le->running || !pcm || !out)
        return -1;

    samples = frames * le->config.channels;
    if (samples * 2 > out_cap)
        return -2;

    /* The platform LC3 encoder is called here; without it the PCM is packed. */
    memcpy(out, pcm, samples * 2);
    le->stats.frames_sent++;
    le->sequence++;
    return (int)(samples * 2);
}

int le_audio_decode(struct le_audio *le, const u8 *frame, u32 len,
                    s16 *pcm, u32 pcm_cap)
{
    if (!le || !le->running || !frame || !pcm)
        return -1;
    if (len > pcm_cap)
        return -2;

    memcpy(pcm, frame, len);
    le->stats.frames_received++;

    if (le->config.presentation_delay_us > le->config.transport_latency_ms * 1000)
        le->stats.late_frames++;

    return (int)(len / 2);
}

const struct le_audio_stats *le_audio_stats(const struct le_audio *le)
{
    return le ? &le->stats : 0;
}
