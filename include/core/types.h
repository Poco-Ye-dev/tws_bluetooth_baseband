/* --- include/core/types.h ---
 *
 * Fixed-width types, endian helpers and the compiler abstraction used by the
 * rest of the tree.  This header is the only place that knows about compiler
 * builtins; everything else includes <core/types.h> and stays portable.
 */

#ifndef __CORE_TYPES_H__
#define __CORE_TYPES_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef volatile u8  vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef unsigned int  uint;
typedef unsigned long ulong;
typedef unsigned char uchar;

/* Address-sized integer, used by allocators and containers. */
typedef uintptr_t uptr;
typedef intptr_t  sptr;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifndef NULL
#define NULL ((void *)0)
#endif

#define STATIC_ASSERT(cond, msg) _Static_assert(cond, #msg)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define clamp(v, lo, hi) min(max((v), (lo)), (hi))

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define ALIGN_UP(x, a)     (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a)   ((x) & ~((a) - 1))

/* Container helper: recover a struct from a pointer to one of its members. */
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* Compile-time assertions that a member lives at a given offset. */
#define BUILD_BUG_ON(cond) _Static_assert(!(cond), #cond)

static inline u16 swap16(u16 v) { return (u16)((v >> 8) | (v << 8)); }
static inline u32 swap32(u32 v)
{
    return ((v & 0x000000ffu) << 24) |
           ((v & 0x0000ff00u) << 8)  |
           ((v & 0x00ff0000u) >> 8)  |
           ((v & 0xff000000u) >> 24);
}

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define cpu_to_le16(v) swap16(v)
#define cpu_to_le32(v) swap32(v)
#define le16_to_cpu(v) swap16(v)
#define le32_to_cpu(v) swap32(v)
#define cpu_to_be16(v) (v)
#define cpu_to_be32(v) (v)
#define be16_to_cpu(v) (v)
#define be32_to_cpu(v) (v)
#else
#define cpu_to_le16(v) (v)
#define cpu_to_le32(v) (v)
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#define cpu_to_be16(v) swap16(v)
#define cpu_to_be32(v) swap32(v)
#define be16_to_cpu(v) swap16(v)
#define be32_to_cpu(v) swap32(v)
#endif

/* Barriers, used by lock-free queues and DMA hand-off. */
#define barrier()        __asm__ __volatile__("" ::: "memory")
#define read_barrier()   barrier()
#define write_barrier()  barrier()

#endif /* __CORE_TYPES_H__ */
