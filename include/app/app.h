#ifndef __APP_APP_H__
#define __APP_APP_H__

#include <core/types.h>
#include <core/bitops.h>
#include <system/event.h>

/*
 * Application framework.  A product links exactly one app descriptor; the
 * framework initialises it, drains the event queue and dispatches mode
 * changes.  Keeping the loop here lets every product share the power and
 * watchdog handling.
 */

#define APP_EVENT_BOOT   BIT(0)
#define APP_EVENT_BT     BIT(1)
#define APP_EVENT_KEY    BIT(2)
#define APP_EVENT_AUDIO  BIT(3)
#define APP_EVENT_TIMER  BIT(4)
#define APP_EVENT_POWER  BIT(5)

struct app_ops {
    const char *name;
    int (*init)(void);
    int (*event)(u32 events);
    int (*deinit)(void);
};

int app_register(const struct app_ops *ops);
int app_boot(void);
void app_post(u32 events);
struct event_group *app_events(void);
const char *app_name(void);

#endif /* __APP_APP_H__ */
