#include <app/app.h>
#include <core/printk.h>
#include <core/mem.h>
#include <core/string.h>

static int earphone_init(void)
{
    struct mem_info info;

    mem_info(&info);
    pr_info("earphone: heap %u/%u bytes\n", (u32)info.used, (u32)info.total);
    return 0;
}

static int earphone_event(u32 events)
{
    if (events & APP_EVENT_BOOT)
        pr_info("earphone: boot event\n");

    if (events & APP_EVENT_KEY)
        pr_info("earphone: key event\n");

    if (events & APP_EVENT_BT)
        pr_info("earphone: bt event\n");

    return 0;
}

static const struct app_ops earphone_app = {
    .name = "earphone",
    .init = earphone_init,
    .event = earphone_event,
};

void app_main(void)
{
    app_register(&earphone_app);
    app_boot();
}
