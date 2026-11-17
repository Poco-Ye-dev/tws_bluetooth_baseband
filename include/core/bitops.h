#ifndef __CORE_BITOPS_H__
#define __CORE_BITOPS_H__

#include <core/types.h>

#define BIT(n)          (1u << (n))
#define BIT64(n)        (1ull << (n))
#define GENMASK(h, l)   (((~0u) << (l)) & (~0u >> (31 - (h))))
#define GENMASK_ULL(h, l) (((~0ull) << (l)) & (~0ull >> (63 - (h))))

static inline void set_bit(int nr, volatile u32 *addr)
{
    *addr |= BIT(nr);
}

static inline void clear_bit(int nr, volatile u32 *addr)
{
    *addr &= ~BIT(nr);
}

static inline void change_bit(int nr, volatile u32 *addr)
{
    *addr ^= BIT(nr);
}

static inline int test_bit(int nr, const volatile u32 *addr)
{
    return!!(*addr & BIT(nr));
}

static inline int test_and_set_bit(int nr, volatile u32 *addr)
{
    u32 old = *addr;
    *addr = old | BIT(nr);
    return !!(old & BIT(nr));
}

static inline int test_and_clear_bit(int nr, volatile u32 *addr)
{
    u32 old = *addr;
    *addr = old & ~BIT(nr);
    return !!(old & BIT(nr));
}

static inline int fls(u32 word)
{
    return word ? 32 - __builtin_clz(word) : 0;
}

static inline int ffs(u32 word)
{
    return word ? __builtin_ctz(word) : -1;
}

static inline u32 roundup_pow_of_two(u32 v)
{
    if (v <= 1)
        return 1;
    return 1u << fls(v - 1);
}

/* Iterate over set bits: for_each_set_bit(bit, mask) { ... } */
#define for_each_set_bit(bit, word)                                    \
    for (u32 __m = (word); __m ? ((bit) = __builtin_ctz(__m), 1) : 0; \
         __m &= ~BIT(bit))

#endif /* __CORE_BITOPS_H__ */
