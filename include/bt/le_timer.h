#ifndef __BT_LE_TIMER_H__
#define __BT_LE_TIMER_H__

#include <core/types.h>

/*
 * Slot timer used to arm an anchor window.  A timer fires when the slot
 * counter reaches its deadline; the callback opens the HW RX window or
 * closes it on timeout.
 */

#define LE_TIMER_MAX 16

typedef void (*le_timer_cb_t)(void *arg);

struct le_timer {
    bool used;
    bool armed;
    u32 deadline;
    le_timer_cb_t cb;
    void *arg;
};

void le_timer_init(void);
struct le_timer *le_timer_alloc(le_timer_cb_t cb, void *arg);
void le_timer_free(struct le_timer *timer);
int le_timer_arm(struct le_timer *timer, u32 deadline);
int le_timer_stop(struct le_timer *timer);
void le_timer_tick(u32 slot);

#endif /* __BT_LE_TIMER_H__ */
