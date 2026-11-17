#ifndef __AUDIO_TEST_TOOLS_H__
#define __AUDIO_TEST_TOOLS_H__

#include <core/types.h>

/*
 * Built-in signal generators and measurements used by production line tests
 * and by the audio team during bring-up.
 */

struct audio_test_stats {
    u32 frames;
    u32 underruns;
    u32 overruns;
    u32 max_latency_us;
    s32 peak;
    s32 rms;
};

void audio_test_sine(s16 *dst, u32 samples, u32 sample_rate, u32 freq_hz,
                     s16 amplitude, u32 *phase);
void audio_test_noise(s16 *dst, u32 samples, u32 *seed, s16 amplitude);
void audio_test_square(s16 *dst, u32 samples, u32 sample_rate, u32 freq_hz,
                       s16 amplitude, u32 *phase);
void audio_test_analyze(const s16 *data, u32 samples, struct audio_test_stats *stats);
void audio_test_mark_underrun(void);
void audio_test_mark_overrun(void);
void audio_test_reset(void);
struct audio_test_stats *audio_test_get_stats(void);

#endif /* __AUDIO_TEST_TOOLS_H__ */
