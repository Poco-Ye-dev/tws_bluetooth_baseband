#include <core/math.h>

/*
 * Fixed point helpers.  The decibel conversions use the standard log10 /
 * pow10 identities evaluated in integer arithmetic, which is accurate enough
 * for gain staging and avoids pulling in libm.
 */

static const u32 pow10_q15[13] = {
    32768, 41253, 51936, 65392, 82323, 103644, 130591, 164433,
    207014, 260641, 328152, 413149, 520175,
};

static u32 pow10_q15_at(u32 index)
{
    return pow10_q15[index > 12 ? 12 : index];
}

s16 db_to_gain_q15(s16 db)
{
    bool negative = db < 0;
    u32 whole;
    u32 frac;

    if (db == 0)
        return 32767;

    if (db < 0)
        db = (s16)-db;

    whole = (u32)db / 6;
    frac = (u32)db % 6;

    if (whole > 12)
        return negative ? 0 : 32767;

    u32 base = pow10_q15_at(whole);
    u32 next = pow10_q15_at(whole + 1);
    u32 gain = base + ((next - base) * frac) / 6;

    if (gain > 32767)
        gain = 32767;

    return negative ? (s16)(32768 / (gain ? gain : 1)) : (s16)gain;
}

s16 gain_q15_to_db(s16 q15)
{
    u32 gain = (u32)(q15 < 0 ? -q15 : q15);
    u32 db = 0;

    if (gain == 0)
        return -96;
    if (gain >= 32767)
        return 0;

    while (gain < 32768) {
        gain = gain * 2;
        db += 6;
    }

    return (s16)(q15 < 0 ? -(s16)db : (s16)db);
}

u32 db10_to_linear(s32 milli_db)
{
    bool negative = milli_db < 0;
    u32 value = (u32)(negative ? -milli_db : milli_db);
    u32 whole = value / 6000;
    u32 rem = value % 6000;
    u32 result;

    if (whole > 12)
        return negative ? 0 : 0xFFFFFFFFu;

    result = pow10_q15_at(whole);
    result += ((pow10_q15_at(whole + 1) - pow10_q15_at(whole)) * rem) / 6000;

    return negative ? (u32)((32768u * 32768u) / result) : result;
}

u32 isqrt(u32 v)
{
    u32 root = 0;
    u32 bit = 1u << 30;

    while (bit > v)
        bit >>= 2;

    while (bit) {
        if (v >= root + bit) {
            v -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

s32 log2_int(u32 v)
{
    if (v == 0)
        return -1;

    s32 shift = 0;
    while (v >>= 1)
        shift++;
    return shift;
}
