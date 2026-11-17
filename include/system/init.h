#ifndef __SYSTEM_INIT_H__
#define __SYSTEM_INIT_H__

#include <core/types.h>
#include <core/compiler.h>

/*
 * Ordered initialisation without hard-coded call lists.
 *
 * Every subsystem registers a function in the .initcall.<level> section; the
 * linker sorts the sections alphabetically so level "1" runs before "2".
 * Board files therefore only list hardware, never software ordering.
 */

enum init_level {
    INIT_LEVEL_EARLY = 1,
    INIT_LEVEL_CORE,
    INIT_LEVEL_DRIVER,
    INIT_LEVEL_MEDIA,
    INIT_LEVEL_APP,
    INIT_LEVEL_LATE,
};

typedef int (*init_fn_t)(void);

struct initcall {
    init_fn_t fn;
    const char *name;
    u8 level;
};

#define __initcall(fn, level)                                          \
    static const struct initcall __initcall_##fn                       \
        __section("initcall") __used = { fn, #fn, (u8)(level) }

#define early_initcall(fn)  __initcall(fn, 1)
#define core_initcall(fn)   __initcall(fn, 2)
#define driver_initcall(fn) __initcall(fn, 3)
#define media_initcall(fn)  __initcall(fn, 4)
#define app_initcall(fn)    __initcall(fn, 5)
#define late_initcall(fn)   __initcall(fn, 6)

int init_run(enum init_level level);
int init_run_all(void);

#endif /* __SYSTEM_INIT_H__ */
