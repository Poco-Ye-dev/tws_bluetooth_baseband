#ifndef __SYSTEM_EVENT_H__
#define __SYSTEM_EVENT_H__

#include <core/types.h>
#include <core/list.h>

/*
 * Event groups: a task waits for any (or all) of a set of bits to be posted.
 * This is the primary hand-off from interrupt handlers to the application.
 */

#define EVENT_WAIT_ANY 0
#define EVENT_WAIT_ALL 1
#define EVENT_WAIT_FOREVER 0xFFFFFFFFu

struct event_group {
    volatile u32 bits;
    struct list_head waiters;
    const char *name;
};

void event_init(struct event_group *group, const char *name);
u32  event_post(struct event_group *group, u32 bits);
u32  event_clear(struct event_group *group, u32 bits);
u32  event_get(const struct event_group *group);

/*
 * Wait until mask is satisfied.  Returns the matching bits, or 0 on timeout.
 */
u32 event_wait(struct event_group *group, u32 mask, u32 mode, u32 timeout_ms);

#endif /* __SYSTEM_EVENT_H__ */
