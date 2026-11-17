#ifndef __AUDIO_COMMON_H__
#define __AUDIO_COMMON_H__

#include <core/types.h>
#include <media/buffer.h>

/*
 * Sample level helpers shared by every audio module.  They are header-only
 * friendly but live in a translation unit so callers do not pay for code size
 * twice.
 */

void audio_convert_s16_s24(const s16 *in, u8 *out, u32 samples);
void audio_convert_s24_s16(const u8 *in, s16 *out, u32 samples);
void audio_apply_gain(s16 *data, u32 samples, s16 gain_q15);
void audio_apply_fade_in(s16 *data, u32 samples, u32 ramp_samples);
void audio_apply_fade_out(s16 *data, u32 samples, u32 ramp_samples);
void audio_dc_remove(s16 *data, u32 samples, s32 *state);
void audio_channel_map(s16 *out, const s16 *in, u8 out_channels,
                       u8 in_channels, u32 frames);

/* Mix two buffers preserving headroom; used by the framework mixer. */
void audio_mix_add(s16 *dst, const s16 *src, u32 samples);

#endif /* __AUDIO_COMMON_H__ */
