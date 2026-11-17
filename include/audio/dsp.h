#ifndef __AUDIO_DSP_H__
#define __AUDIO_DSP_H__

#include <core/types.h>

/*
 * Small fixed point DSP toolkit shared by effects and the audio path.
 *
 * All state is explicit and allocation free, so the functions are safe to run
 * from the audio callback without touching the heap.
 */

struct biquad {
    s32 b0, b1, b2, a1, a2;
    s64 z1, z2;
};

enum biquad_type {
    BIQUAD_PEAKING = 0,
    BIQUAD_LOWPASS,
    BIQUAD_HIGHPASS,
};

void biquad_design(struct biquad *bq, enum biquad_type type, u32 sample_rate,
                   u32 freq, s16 gain_db, u16 q10);
s16 biquad_process(struct biquad *bq, s16 sample);
void biquad_reset(struct biquad *bq);

struct limiter {
    s32 threshold_q15;
    s32 attack_q15;
    s32 release_q15;
    s32 envelope;
};

void limiter_init(struct limiter *lim, s32 threshold_q15);
s16 limiter_process(struct limiter *lim, s16 sample);

void dsp_mix_s16(s16 *out, const s16 *a, const s16 *b,
                 s16 gain_a_q15, s16 gain_b_q15, u32 samples);

u32 dsp_resample_linear(const s16 *in, u32 in_frames, u32 in_rate,
                        u32 out_rate, s16 *out, u32 out_cap);

struct agc {
    s32 target_q15;
    s32 gain_q15;
    s32 decay;
};

void agc_init(struct agc *agc, s32 target_q15);
s16 agc_process(struct agc *agc, s16 sample);

struct vad {
    s32 threshold;
    u8 hangover;
    u8 counter;
};

void vad_init(struct vad *vad, s32 threshold, u8 hangover);
bool vad_process(struct vad *vad, const s16 *samples, u32 count);

#endif /* __AUDIO_DSP_H__ */
