#include <driver/watchdog.h>

static struct watchdog_device *wdt_dev;

int watchdog_register(struct watchdog_device *wdt)
{
    if (!wdt || !wdt->start || !wdt->feed)
        return -EINVAL;
    wdt_dev = wdt;
    return 0;
}

struct watchdog_device *watchdog_get(const char *name)
{
    (void)name;
    return wdt_dev;
}

int watchdog_start(struct watchdog_device *wdt, u32 timeout_ms)
{
    if (!wdt || !timeout_ms)
        return -EINVAL;

    int ret = wdt->start(wdt, timeout_ms);
    if (!ret) {
        wdt->timeout_ms = timeout_ms;
        wdt->enabled = true;
    }
    return ret;
}

int watchdog_feed(struct watchdog_device *wdt)
{
    if (!wdt || !wdt->enabled)
        return -EPERM;
    return wdt->feed(wdt);
}

int watchdog_stop(struct watchdog_device *wdt)
{
    if (!wdt || !wdt->stop)
        return -ENOSYS;

    int ret = wdt->stop(wdt);
    if (!ret)
        wdt->enabled = false;
    return ret;
}

bool watchdog_enabled(struct watchdog_device *wdt)
{
    return wdt && wdt->enabled;
}
