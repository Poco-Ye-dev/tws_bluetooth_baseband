#include <system/event.h>
#include <system/task.h>
#include <system/timer.h>
#include <core/errno.h>

struct event_waiter {
    struct list_head node;
    struct task *task;
    u32 mask;
    u32 mode;
};

void event_init(struct event_group *group, const char *name)
{
    group->bits = 0;
    INIT_LIST_HEAD(&group->waiters);
    group->name = name;
}

u32 event_get(const struct event_group *group)
{
    return group->bits;
}

u32 event_clear(struct event_group *group, u32 bits)
{
    u32 old = group->bits;
    group->bits &= ~bits;
    return old;
}

static bool waiter_satisfied(const struct event_waiter *w, u32 bits)
{
    if (w->mode == EVENT_WAIT_ALL)
        return (bits & w->mask) == w->mask;
    return (bits & w->mask) != 0;
}

u32 event_post(struct event_group *group, u32 bits)
{
    struct event_waiter *w, *tmp;
    u32 matched = 0;

    group->bits |= bits;

    list_for_each_entry_safe(w, tmp, &group->waiters, node) {
        if (!waiter_satisfied(w, group->bits))
            continue;

        matched |= w->mask & group->bits;
        list_del_init(&w->node);
        task_unblock(w->task);
    }
    return matched;
}

u32 event_wait(struct event_group *group, u32 mask, u32 mode, u32 timeout_ms)
{
    struct event_waiter waiter;
    u32 start = system_ticks();
    u32 result;

    waiter.task = task_self();
    waiter.mask = mask;
    waiter.mode = mode;
    waiter.node.next = waiter.node.prev = NULL;

    for (;;) {
        if (waiter_satisfied(&waiter, group->bits)) {
            result = group->bits & mask;
            return result;
        }

        if (timeout_ms == EVENT_WAIT_FOREVER)
            goto block;

        if ((s32)(system_ticks() - start) >= (s32)timeout_ms)
            return 0;

    block:
        list_add_tail(&waiter.node, &group->waiters);
        task_block();

        if (waiter.node.next)
            list_del_init(&waiter.node);
    }
}
