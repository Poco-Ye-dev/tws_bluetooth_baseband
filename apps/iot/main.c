#include <app/app.h>
#include <core/printk.h>

static int iot_init(void)
{
    pr_info("iot: init\n");
    return 0;
}

static int iot_event(u32 events)
{
    if (events & APP_EVENT_TIMER)
        pr_info("iot: timer event\n");
    return 0;
}

static const struct app_ops iot_app = {
    .name = "iot",
    .init = iot_init,
    .event = iot_event,
};

void app_main(void)
{
    app_register(&iot_app);
    app_boot();
}
