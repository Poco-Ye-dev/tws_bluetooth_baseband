#ifndef __CORE_COMPILER_H__
#define __CORE_COMPILER_H__

#include <core/types.h>

#if defined(__clang__) || defined(__GNUC__)
#define __unused        __attribute__((unused))
#define __maybe_unused  __attribute__((unused))
#define __section(s)    __attribute__((section(s)))
#define __aligned(n)    __attribute__((aligned(n)))
#define __packed        __attribute__((packed))
#define __noreturn      __attribute__((noreturn))
#define __weak          __attribute__((weak))
#define __alias(sym)    __attribute__((alias(#sym)))
#define __used          __attribute__((used))
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#define unreachable()   __builtin_unreachable()
#else
#error "unsupported compiler"
#endif

#if __has_attribute(fallthrough)
#define fallthrough() __attribute__((fallthrough))
#else
#define fallthrough() ((void)0)
#endif

#define __stringify_1(x) #x
#define __stringify(x)   __stringify_1(x)
#define __concat(a, b)   a##b

#endif /* __CORE_COMPILER_H__ */
