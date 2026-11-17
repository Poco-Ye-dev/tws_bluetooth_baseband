#include <bt/le_timer.h>
#include <core/string.h>

static struct le_timer timers[LE_TIMER_MAX];

void le_timer_init(void)
{
    memset(timers, 0, sizeof(timers));
}

struct le_timer *le_timer_alloc(le_timer_cb_t cb, void *arg)
{
    for (unsigned int i = 0; i < LE_TIMER_MAX; i++) {
        if (timers[i].used)
            continue;
        timers[i].used = true;
        timers[i].armed = false;
        timers[i].cb = cb;
        timers[i].arg = arg;
        return &timers[i];
    }
    return NULL;
}

void le_timer_free(struct le_timer *timer)
{
    if (timer)
        memset(timer, 0, sizeof(*timer));
}

int le_timer_arm(struct le_timer *timer, u32 deadline)
{
    if (!timer || !timer->used)
        return -1;
    timer->deadline = deadline;
    timer->armed = true;
    return 0;
}

int le_timer_stop(struct le_timer *timer)
{
    if (!timer)
        return -1;
    timer->armed = false;
    return 0;
}

void le_timer_tick(u32 slot)
{
    for (unsigned int i = 0; i < LE_TIMER_MAX; i++) {
        struct le_timer *timer = &timers[i];
        if (!timer->used || !timer->armed)
            continue;
        if ((s32)(slot - timer->deadline) < 0)
            continue;

        timer->armed = false;
        if (timer->cb)
            timer->cb(timer->arg);
    }
}
