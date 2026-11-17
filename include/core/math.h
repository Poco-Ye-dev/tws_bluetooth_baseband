#ifndef __CORE_MATH_H__
#define __CORE_MATH_H__

#include <core/types.h>

/*
 * The audio pipeline works in fixed point on parts without an FPU.  Keep the
 * helpers here so effects and codecs share one definition of saturation and
 * gain conversion.
 */

#define Q15_ONE   32768
#define Q15_MAX   32767
#define Q15_MIN   (-32768)

#define Q31_ONE   2147483648LL

static inline s16 sat16(s32 v)
{
    if (v > 32767)
        return 32767;
    if (v < -32768)
        return -32768;
    return (s16)v;
}

static inline s32 sat32(s64 v)
{
    if (v > 2147483647LL)
        return 2147483647;
    if (v < -2147483648LL)
        return (s32)(-2147483648LL);
    return (s32)v;
}

static inline s16 q15_mul(s16 a, s16 b)
{
    return sat16(((s32)a * (s32)b) >> 15);
}

static inline s32 abs32(s32 v)
{
    return v < 0 ? -v : v;
}

/* Linear gain in Q15 to decibels, used by the EQ editor. */
s16 gain_q15_to_db(s16 q15);
s16 db_to_gain_q15(s16 db);

/* 10^(x/20) approximated with a table, x in 1/16 dB steps. */
u32 db10_to_linear(s32 milli_db);

u32 isqrt(u32 v);
s32 log2_int(u32 v);

static inline s32 lerp(s32 a, s32 b, u32 weight_q15)
{
    return a + (s32)(((s64)(b - a) * weight_q15) >> 15);
}

#endif /* __CORE_MATH_H__ */
