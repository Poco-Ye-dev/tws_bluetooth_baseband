#ifndef __CORE_ATOMIC_H__
#define __CORE_ATOMIC_H__

#include <core/types.h>

/*
 * Atomic operations used by lock-free queues and reference counters.  The
 * compiler builtins expand to the smallest instruction sequence the core
 * offers; no libatomic is required.
 */

typedef struct {
    volatile s32 counter;
} atomic_t;

#define ATOMIC_INIT(v) { .counter = (v) }

static inline s32 atomic_read(const atomic_t *a)
{
    return __atomic_load_n(&a->counter, __ATOMIC_RELAXED);
}

static inline void atomic_set(atomic_t *a, s32 v)
{
    __atomic_store_n(&a->counter, v, __ATOMIC_RELAXED);
}

static inline void atomic_add(atomic_t *a, s32 v)
{
    __atomic_add_fetch(&a->counter, v, __ATOMIC_SEQ_CST);
}

static inline void atomic_sub(atomic_t *a, s32 v)
{
    __atomic_sub_fetch(&a->counter, v, __ATOMIC_SEQ_CST);
}

static inline s32 atomic_add_return(atomic_t *a, s32 v)
{
    return __atomic_add_fetch(&a->counter, v, __ATOMIC_SEQ_CST);
}

static inline s32 atomic_sub_return(atomic_t *a, s32 v)
{
    return __atomic_sub_fetch(&a->counter, v, __ATOMIC_SEQ_CST);
}

static inline s32 atomic_inc_return(atomic_t *a)
{
    return atomic_add_return(a, 1);
}

static inline s32 atomic_dec_return(atomic_t *a)
{
    return atomic_sub_return(a, 1);
}

static inline bool atomic_dec_and_test(atomic_t *a)
{
    return atomic_sub_return(a, 1) == 0;
}

static inline s32 atomic_cmpxchg(atomic_t *a, s32 old, s32 new_value)
{
    __atomic_compare_exchange_n(&a->counter, &old, new_value, false,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return old;
}

static inline s32 atomic_xchg(atomic_t *a, s32 v)
{
    return __atomic_exchange_n(&a->counter, v, __ATOMIC_SEQ_CST);
}

#endif /* __CORE_ATOMIC_H__ */
