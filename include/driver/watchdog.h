#ifndef __DRIVER_WATCHDOG_H__
#define __DRIVER_WATCHDOG_H__

#include <core/types.h>
#include <core/errno.h>

struct watchdog_device {
    const char *name;
    u32 timeout_ms;
    bool enabled;
    int (*start)(struct watchdog_device *wdt, u32 timeout_ms);
    int (*feed)(struct watchdog_device *wdt);
    int (*stop)(struct watchdog_device *wdt);
    void *priv;
};

int watchdog_register(struct watchdog_device *wdt);
struct watchdog_device *watchdog_get(const char *name);
int watchdog_start(struct watchdog_device *wdt, u32 timeout_ms);
int watchdog_feed(struct watchdog_device *wdt);
int watchdog_stop(struct watchdog_device *wdt);
bool watchdog_enabled(struct watchdog_device *wdt);

#endif /* __DRIVER_WATCHDOG_H__ */
