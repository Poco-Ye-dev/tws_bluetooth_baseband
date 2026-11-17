#include <app/app.h>
#include <system/task.h>
#include <core/printk.h>
#include <core/string.h>
#include <core/errno.h>

#define APP_NAME_MAX 16

static const struct app_ops *app;
static struct event_group events;
static bool booted;

int app_register(const struct app_ops *ops)
{
    if (!ops || !ops->init)
        return -EINVAL;
    if (app)
        return -EBUSY;

    app = ops;
    return 0;
}

struct event_group *app_events(void)
{
    return &events;
}

void app_post(u32 bits)
{
    event_post(&events, bits);
}

const char *app_name(void)
{
    return app ? app->name : "(none)";
}

static void app_loop(void *arg)
{
    (void)arg;

    while (1) {
        u32 events_seen = event_wait(&events, 0xFFFFFFFFu, EVENT_WAIT_ANY,
                                     EVENT_WAIT_FOREVER);
        event_clear(&events, events_seen);

        if (app->event) {
            int ret = app->event(events_seen);
            if (ret)
                pr_err("%s: event 0x%x failed: %d\n", app->name,
                       events_seen, ret);
        }
    }
}

int app_boot(void)
{
    int ret;

    if (!app)
        return -ENODEV;
    if (booted)
        return -EALREADY;

    event_init(&events, "app");

    ret = app->init();
    if (ret) {
        pr_err("%s: init failed: %d\n", app->name, ret);
        return ret;
    }

    booted = true;
    pr_info("app: %s ready\n", app->name);

    if (!task_create("app", app_loop, NULL, TASK_PRIO_NORMAL, 2048))
        return -ENOMEM;

    app_post(APP_EVENT_BOOT);
    task_start();
    return 0;
}
