#ifndef __CORE_PRINTK_H__
#define __CORE_PRINTK_H__

#include <core/compiler.h>

/*
 * Kernel logging.  The two entry points differ only in severity:
 *
 *   printk()  always emitted
 *   pr_err()/pr_warn()/pr_info()/pr_dbg()  carry a level prefix and can be
 *   compiled out with CONFIG_LOG_LEVEL.
 */

enum log_level {
    LOG_LEVEL_ERR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DBG,
};

void printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void vprintk(const char *fmt, __builtin_va_list ap);

#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL LOG_LEVEL_INFO
#endif

#define pr_err(fmt, ...) \
    printk("[E] " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
    do { if (CONFIG_LOG_LEVEL >= LOG_LEVEL_WARN) printk("[W] " fmt, ##__VA_ARGS__); } while (0)
#define pr_info(fmt, ...) \
    do { if (CONFIG_LOG_LEVEL >= LOG_LEVEL_INFO) printk("[I] " fmt, ##__VA_ARGS__); } while (0)
#define pr_dbg(fmt, ...) \
    do { if (CONFIG_LOG_LEVEL >= LOG_LEVEL_DBG) printk("[D] " fmt, ##__VA_ARGS__); } while (0)

#endif /* __CORE_PRINTK_H__ */
