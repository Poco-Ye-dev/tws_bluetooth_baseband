#ifndef __SYSTEM_TIMER_H__
#define __SYSTEM_TIMER_H__

#include <core/types.h>
#include <core/list.h>

/*
 * Software timers driven by the 1 kHz system tick.  A timer may be periodic
 * or one shot; callbacks run in interrupt context and must stay short.
 */

typedef void (*timer_cb_t)(void *arg);

struct timer {
    struct list_head node;
    const char *name;
    timer_cb_t callback;
    void *arg;
    u32 period_ms;
    u32 expires_ms;
    bool periodic;
    bool active;
};

void timer_init(struct timer *timer, const char *name, timer_cb_t cb, void *arg);
int  timer_start(struct timer *timer, u32 delay_ms, bool periodic);
void timer_stop(struct timer *timer);
bool timer_active(const struct timer *timer);

/* Called from the tick handler. */
void timer_process(u32 now_ms);

u32 system_ticks(void);
void system_tick(void);

#endif /* __SYSTEM_TIMER_H__ */
