#ifndef __CORE_ASSERT_H__
#define __CORE_ASSERT_H__

#include <core/compiler.h>
#include <core/printk.h>

/*
 * Fatal conditions.  BUG() is compiled in release builds; ASSERT() follows
 * CONFIG_ASSERT_ENABLE so the cost disappears when the option is off.
 */

void panic(const char *file, int line, const char *expr) __noreturn;

#define BUG_ON(cond)                                        \
    do {                                                    \
        if (unlikely(cond))                                 \
            panic(__FILE__, __LINE__, #cond);               \
    } while (0)

#define WARN_ON(cond)                                       \
    ({                                                      \
        int __w = !!(cond);                                 \
        if (unlikely(__w))                                  \
            printk("[W] %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        __w;                                                \
    })

#ifdef CONFIG_ASSERT_ENABLE
#define ASSERT(cond) BUG_ON(!(cond))
#else
#define ASSERT(cond) do { } while (0)
#endif

#endif /* __CORE_ASSERT_H__ */
