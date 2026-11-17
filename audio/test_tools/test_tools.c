#include <audio/test_tools.h>
#include <core/math.h>
#include <core/string.h>

static struct audio_test_stats stats;

void audio_test_reset(void)
{
    memset(&stats, 0, sizeof(stats));
}

struct audio_test_stats *audio_test_get_stats(void)
{
    return &stats;
}

void audio_test_sine(s16 *dst, u32 samples, u32 sample_rate, u32 freq_hz,
                     s16 amplitude, u32 *phase)
{
    u32 step = (u32)(((u64)freq_hz << 16) / (sample_rate ? sample_rate : 1));
    s32 ph = phase ? (s32)*phase : 0;

    /* Sine via a 64 entry quarter wave table keeps the core free of libm. */
    static const s16 sine_q15[65] = {
        0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
        23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609,
        32767, 32609, 32137, 31356, 30273, 28898, 27245, 25329,
        23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212,
        0, -3212, -6393, -9512, -12539, -15446, -18204, -20787,
        -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
        -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
        -23170, -20787, -18204, -15446, -12539, -9512, -6393, -3212,
        0,
    };

    for (u32 i = 0; i < samples; i++) {
        u32 index = ((u32)ph >> 10) & 0x3F;
        dst[i] = sat16(((s32)sine_q15[index] * amplitude) >> 15);
        ph += (s32)step;
    }
    if (phase)
        *phase = (u32)ph;
}

void audio_test_noise(s16 *dst, u32 samples, u32 *seed, s16 amplitude)
{
    u32 state = seed ? *seed : 0x12345678u;

    for (u32 i = 0; i < samples; i++) {
        state = state * 1664525u + 1013904223u;
        s32 value = (s32)(state >> 16) - 32768;
        dst[i] = sat16((value * amplitude) >> 15);
    }
    if (seed)
        *seed = state;
}

void audio_test_square(s16 *dst, u32 samples, u32 sample_rate, u32 freq_hz,
                       s16 amplitude, u32 *phase)
{
    u32 step = (u32)(((u64)freq_hz << 16) / (sample_rate ? sample_rate : 1));
    u32 ph = phase ? *phase : 0;

    for (u32 i = 0; i < samples; i++) {
        dst[i] = (ph & 0x8000) ? -amplitude : amplitude;
        ph += step;
    }
    if (phase)
        *phase = ph;
}

void audio_test_analyze(const s16 *data, u32 samples, struct audio_test_stats *out)
{
    s64 sum = 0;
    s32 peak = 0;

    if (!data || !out)
        return;

    for (u32 i = 0; i < samples; i++) {
        s32 value = abs32(data[i]);
        sum += (s64)data[i] * data[i];
        if (value > peak)
            peak = value;
    }

    out->frames++;
    out->peak = peak;
    out->rms = samples ? (s32)isqrt((u32)(sum / samples)) : 0;
}

void audio_test_mark_underrun(void)
{
    stats.underruns++;
}

void audio_test_mark_overrun(void)
{
    stats.overruns++;
}
