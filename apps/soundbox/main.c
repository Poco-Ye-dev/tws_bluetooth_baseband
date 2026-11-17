#include <app/app.h>
#include <core/printk.h>

static int soundbox_init(void)
{
    pr_info("soundbox: init\n");
    return 0;
}

static int soundbox_event(u32 events)
{
    if (events & APP_EVENT_AUDIO)
        pr_info("soundbox: audio event\n");
    return 0;
}

static const struct app_ops soundbox_app = {
    .name = "soundbox",
    .init = soundbox_init,
    .event = soundbox_event,
};

void app_main(void)
{
    app_register(&soundbox_app);
    app_boot();
}
