#include <system/timer.h>
#include <core/errno.h>
#include <system/task.h>
#include <core/list.h>
#include <core/printk.h>

/*
 * Software timer queue.  Timers are kept in a flat list; with the small
 * number used by the firmware a heap would cost more than it saves.  The
 * tick handler runs the queue at 1 kHz.
 */

static LIST_HEAD(timer_list);
static u32 tick_count;

void timer_init(struct timer *timer, const char *name, timer_cb_t cb, void *arg)
{
    timer->node.next = NULL;
    timer->node.prev = NULL;
    timer->name = name;
    timer->callback = cb;
    timer->arg = arg;
    timer->period_ms = 0;
    timer->expires_ms = 0;
    timer->periodic = false;
    timer->active = false;
}

int timer_start(struct timer *timer, u32 delay_ms, bool periodic)
{
    if (!timer || !timer->callback)
        return -EINVAL;

    if (timer->active)
        timer_stop(timer);

    timer->period_ms = delay_ms;
    timer->periodic = periodic;
    timer->expires_ms = system_ticks() + delay_ms;
    timer->active = true;
    list_add_tail(&timer->node, &timer_list);
    return 0;
}

void timer_stop(struct timer *timer)
{
    if (!timer || !timer->active)
        return;

    list_del_init(&timer->node);
    timer->active = false;
}

bool timer_active(const struct timer *timer)
{
    return timer && timer->active;
}

void timer_process(u32 now_ms)
{
    struct timer *timer, *tmp;

    list_for_each_entry_safe(timer, tmp, &timer_list, node) {
        if ((s32)(now_ms - timer->expires_ms) < 0)
            continue;

        if (timer->periodic) {
            timer->expires_ms = now_ms + timer->period_ms;
        } else {
            list_del_init(&timer->node);
            timer->active = false;
        }
        timer->callback(timer->arg);
    }
}

u32 system_ticks(void)
{
    return tick_count;
}

void system_tick(void)
{
    tick_count++;
    timer_process(tick_count);
    task_tick();
}
