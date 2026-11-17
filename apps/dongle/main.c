#include <app/app.h>
#include <core/printk.h>

static int dongle_init(void)
{
    pr_info("dongle: init\n");
    return 0;
}

static int dongle_event(u32 events)
{
    if (events & APP_EVENT_BT)
        pr_info("dongle: bt event\n");
    return 0;
}

static const struct app_ops dongle_app = {
    .name = "dongle",
    .init = dongle_init,
    .event = dongle_event,
};

void app_main(void)
{
    app_register(&dongle_app);
    app_boot();
}
