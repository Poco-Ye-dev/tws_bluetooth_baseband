#include <media/adpcm.h>

/*
 * IMA ADPCM (DVI4 variant) reference step table.
 * Step size is intentionally a power-of-two friendly table so the inner loop
 * uses shifts and additions only.
 */

static const s16 step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
    2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
    6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

static const s8 index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

void adpcm_reset(struct adpcm_state *state)
{
    if (!state)
        return;
    state->predictor = 0;
    state->step_index = 0;
}

static s16 decode_nibble(struct adpcm_state *state, u8 nibble)
{
    s32 step = step_table[state->step_index];
    s32 diff = step >> 3;

    if (nibble & 1)
        diff += step >> 2;
    if (nibble & 2)
        diff += step >> 1;
    if (nibble & 4)
        diff += step;
    if (nibble & 8)
        diff = -diff;

    state->predictor += diff;
    if (state->predictor > 32767)
        state->predictor = 32767;
    if (state->predictor < -32768)
        state->predictor = -32768;

    state->step_index += index_table[nibble & 0x0F];
    if (state->step_index < 0)
        state->step_index = 0;
    if (state->step_index > 88)
        state->step_index = 88;

    return (s16)state->predictor;
}

static u8 encode_sample(struct adpcm_state *state, s16 sample)
{
    s32 step = step_table[state->step_index];
    s32 diff = (s32)sample - state->predictor;
    u8 nibble = 0;

    if (diff < 0) {
        nibble = 8;
        diff = -diff;
    }

    s32 temp = step;
    if (diff >= temp) {
        nibble |= 4;
        diff -= temp;
    }
    temp >>= 1;
    if (diff >= temp) {
        nibble |= 2;
        diff -= temp;
    }
    temp >>= 1;
    if (diff >= temp)
        nibble |= 1;

    decode_nibble(state, nibble);
    return nibble;
}

int adpcm_decode(struct adpcm_state *state, const u8 *in, s16 *out, u32 samples)
{
    if (!state || !in || !out)
        return -1;

    for (u32 i = 0; i < samples; i++) {
        u8 byte = in[i >> 1];
        u8 nibble = (i & 1) ? (u8)(byte >> 4) : (u8)(byte & 0x0F);
        out[i] = decode_nibble(state, nibble);
    }
    return (int)samples;
}

int adpcm_encode(struct adpcm_state *state, const s16 *in, u8 *out, u32 samples)
{
    if (!state || !in || !out)
        return -1;

    for (u32 i = 0; i < samples; i += 2) {
        u8 lo = encode_sample(state, in[i]);
        u8 hi = (i + 1 < samples) ? encode_sample(state, in[i + 1]) : 0;
        out[i >> 1] = (u8)(lo | (hi << 4));
    }
    return (int)((samples + 1) / 2);
}
