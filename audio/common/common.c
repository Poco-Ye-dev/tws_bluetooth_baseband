#include <audio/common.h>
#include <core/math.h>

void audio_convert_s16_s24(const s16 *in, u8 *out, u32 samples)
{
    for (u32 i = 0; i < samples; i++) {
        s32 value = (s32)in[i] << 8;
        out[i * 3 + 0] = (u8)(value & 0xFF);
        out[i * 3 + 1] = (u8)((value >> 8) & 0xFF);
        out[i * 3 + 2] = (u8)((value >> 16) & 0xFF);
    }
}

void audio_convert_s24_s16(const u8 *in, s16 *out, u32 samples)
{
    for (u32 i = 0; i < samples; i++) {
        s32 value = (s32)in[i * 3] |
                    ((s32)in[i * 3 + 1] << 8) |
                    ((s32)in[i * 3 + 2] << 16);
        out[i] = sat16(value >> 8);
    }
}

void audio_apply_gain(s16 *data, u32 samples, s16 gain_q15)
{
    for (u32 i = 0; i < samples; i++)
        data[i] = q15_mul(data[i], gain_q15);
}

void audio_apply_fade_in(s16 *data, u32 samples, u32 ramp_samples)
{
    if (!ramp_samples)
        return;
    for (u32 i = 0; i < samples && i < ramp_samples; i++) {
        s16 gain = (s16)(((i + 1) * 32768) / ramp_samples);
        data[i] = q15_mul(data[i], gain);
    }
}

void audio_apply_fade_out(s16 *data, u32 samples, u32 ramp_samples)
{
    if (!ramp_samples)
        return;
    for (u32 i = 0; i < samples && i < ramp_samples; i++) {
        u32 remaining = ramp_samples - i;
        s16 gain = (s16)((remaining * 32768) / ramp_samples);
        data[samples - 1 - i] = q15_mul(data[samples - 1 - i], gain);
    }
}

void audio_dc_remove(s16 *data, u32 samples, s32 *state)
{
    if (!state)
        return;
    for (u32 i = 0; i < samples; i++) {
        s32 value = data[i] - *state;
        *state += value >> 12;
        data[i] = sat16(value);
    }
}

void audio_channel_map(s16 *out, const s16 *in, u8 out_channels,
                       u8 in_channels, u32 frames)
{
    if (!out || !in || !out_channels || !in_channels)
        return;

    for (u32 f = 0; f < frames; f++) {
        for (u8 c = 0; c < out_channels; c++) {
            u8 src = c < in_channels ? c : (u8)(in_channels - 1);
            out[f * out_channels + c] = in[f * in_channels + src];
        }
    }
}

void audio_mix_add(s16 *dst, const s16 *src, u32 samples)
{
    for (u32 i = 0; i < samples; i++)
        dst[i] = sat16((s32)dst[i] + src[i]);
}
