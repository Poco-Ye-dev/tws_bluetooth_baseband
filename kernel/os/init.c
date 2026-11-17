#include <system/init.h>
#include <core/printk.h>

/*
 * The linker concatenates every initcall section into one array bracketed by
 * __start_initcall / __stop_initcall.  Walking it in level order gives a
 * deterministic bring-up without a central list to maintain.
 */

extern const struct initcall __start_initcall[];
extern const struct initcall __stop_initcall[];

static int call_level(u8 level)
{
    int result = 0;

    for (const struct initcall *ic = __start_initcall;
         ic < __stop_initcall; ic++) {
        if (ic->level != level)
            continue;

        int ret = ic->fn();
        if (ret) {
            pr_err("init: %s failed (%d)\n", ic->name, ret);
            result = ret;
        }
    }
    return result;
}

int init_run(enum init_level level)
{
    return call_level((u8)level);
}

int init_run_all(void)
{
    int result = 0;

    for (u8 level = INIT_LEVEL_EARLY; level <= INIT_LEVEL_LATE; level++) {
        int ret = call_level(level);
        if (ret)
            result = ret;
    }
    return result;
}
