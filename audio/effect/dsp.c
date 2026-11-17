#include <audio/dsp.h>
#include <core/math.h>
#include <core/string.h>
#include <math.h>

/*
 * Fixed point DSP primitives.
 *
 * Coefficients are stored in Q20; the transposed direct form II keeps the
 * state in 64 bits which is enough for the dynamic range of a 16 bit stream
 * without overflow checks in the inner loop.
 */

#define Q20_SHIFT 20
#define Q20_ONE   (1 << Q20_SHIFT)

static s32 to_q20(double value)
{
    return (s32)(value * (double)Q20_ONE + (value >= 0 ? 0.5 : -0.5));
}

void biquad_reset(struct biquad *bq)
{
    if (!bq)
        return;
    bq->z1 = 0;
    bq->z2 = 0;
}

void biquad_design(struct biquad *bq, enum biquad_type type, u32 sample_rate,
                   u32 freq, s16 gain_db, u16 q10)
{
    double fs = (double)sample_rate;
    double f0 = (double)freq;
    double q = q10 ? (double)q10 / 1024.0 : 0.707;
    double w0, cos_w0, sin_w0, alpha, a0;
    double b0, b1, b2, a1, a2;

    if (!bq || sample_rate == 0 || freq == 0 || freq * 2 >= sample_rate) {
        if (bq) {
            bq->b0 = Q20_ONE;
            bq->b1 = bq->b2 = bq->a1 = bq->a2 = 0;
            biquad_reset(bq);
        }
        return;
    }

    w0 = 2.0 * 3.14159265358979323846 * f0 / fs;
    cos_w0 = cos(w0);
    sin_w0 = sin(w0);
    alpha = sin_w0 / (2.0 * q);

    switch (type) {
    case BIQUAD_LOWPASS:
        b0 = (1.0 - cos_w0) / 2.0;
        b1 = 1.0 - cos_w0;
        b2 = (1.0 - cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case BIQUAD_HIGHPASS:
        b0 = (1.0 + cos_w0) / 2.0;
        b1 = -(1.0 + cos_w0);
        b2 = (1.0 + cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    default: {
        double amp = pow(10.0, (double)gain_db / 40.0);
        b0 = 1.0 + alpha * amp;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha * amp;
        a0 = 1.0 + alpha / amp;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha / amp;
        break;
    }
    }

    bq->b0 = to_q20(b0 / a0);
    bq->b1 = to_q20(b1 / a0);
    bq->b2 = to_q20(b2 / a0);
    bq->a1 = to_q20(a1 / a0);
    bq->a2 = to_q20(a2 / a0);
    biquad_reset(bq);
}

s16 biquad_process(struct biquad *bq, s16 sample)
{
    s64 x = sample;
    s64 y;

    if (!bq)
        return sample;

    y = (bq->b0 * x + bq->z1) >> Q20_SHIFT;
    bq->z1 = bq->b1 * x - bq->a1 * y + bq->z2;
    bq->z2 = bq->b2 * x - bq->a2 * y;

    return sat16((s32)y);
}

void limiter_init(struct limiter *lim, s32 threshold_q15)
{
    if (!lim)
        return;
    lim->threshold_q15 = threshold_q15 ? threshold_q15 : 30000;
    lim->attack_q15 = 30000;
    lim->release_q15 = 300;
    lim->envelope = 0;
}

s16 limiter_process(struct limiter *lim, s16 sample)
{
    s32 magnitude;
    s32 gain;
    s32 shaped;

    if (!lim)
        return sample;

    magnitude = abs32(sample);
    if (magnitude > lim->envelope)
        lim->envelope = magnitude;
    else
        lim->envelope -= (lim->envelope - magnitude) * (32768 - lim->release_q15) >> 15;

    if (lim->envelope <= lim->threshold_q15)
        return sample;

    gain = (lim->threshold_q15 << 15) / lim->envelope;
    shaped = (sample * gain) >> 15;
    return sat16(shaped);
}

void dsp_mix_s16(s16 *out, const s16 *a, const s16 *b,
                 s16 gain_a_q15, s16 gain_b_q15, u32 samples)
{
    if (!out || !a)
        return;
    if (!b) {
        for (u32 i = 0; i < samples; i++)
            out[i] = q15_mul(a[i], gain_a_q15);
        return;
    }

    for (u32 i = 0; i < samples; i++) {
        s32 mixed = q15_mul(a[i], gain_a_q15) + q15_mul(b[i], gain_b_q15);
        out[i] = sat16(mixed);
    }
}

u32 dsp_resample_linear(const s16 *in, u32 in_frames, u32 in_rate,
                        u32 out_rate, s16 *out, u32 out_cap)
{
    u32 out_frames;
    u64 step;

    if (!in || !out || in_rate == 0 || out_rate == 0)
        return 0;

    out_frames = (u32)(((u64)in_frames * out_rate) / in_rate);
    if (out_frames > out_cap)
        out_frames = out_cap;

    step = ((u64)in_rate << 16) / out_rate;

    for (u32 i = 0; i < out_frames; i++) {
        u64 pos = (u64)i * step;
        u32 index = (u32)(pos >> 16);
        u32 frac = (u32)(pos & 0xFFFF);
        s32 a = index < in_frames ? in[index] : 0;
        s32 b = index + 1 < in_frames ? in[index + 1] : a;
        out[i] = sat16(a + (((b - a) * (s32)frac) >> 16));
    }
    return out_frames;
}

void agc_init(struct agc *agc, s32 target_q15)
{
    if (!agc)
        return;
    agc->target_q15 = target_q15 ? target_q15 : 20000;
    agc->gain_q15 = 32768;
    agc->decay = 16;
}

s16 agc_process(struct agc *agc, s16 sample)
{
    s32 magnitude;
    s32 desired;

    if (!agc)
        return sample;

    magnitude = abs32(sample);
    if (magnitude == 0)
        return sample;

    desired = (agc->target_q15 << 15) / magnitude;
    if (desired > 32768)
        desired = 32768;
    if (desired < 4096)
        desired = 4096;

    agc->gain_q15 += (desired - agc->gain_q15) / agc->decay;
    return sat16((sample * agc->gain_q15) >> 15);
}

void vad_init(struct vad *vad, s32 threshold, u8 hangover)
{
    if (!vad)
        return;
    vad->threshold = threshold ? threshold : 500;
    vad->hangover = hangover ? hangover : 5;
    vad->counter = 0;
}

bool vad_process(struct vad *vad, const s16 *samples, u32 count)
{
    s64 energy = 0;

    if (!vad || !samples || !count)
        return false;

    for (u32 i = 0; i < count; i++)
        energy += abs32(samples[i]);

    energy /= count;
    if (energy >= vad->threshold) {
        vad->counter = vad->hangover;
        return true;
    }

    if (vad->counter) {
        vad->counter--;
        return true;
    }
    return false;
}
