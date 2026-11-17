#ifndef __SYSTEM_WORKQUEUE_H__
#define __SYSTEM_WORKQUEUE_H__

#include <core/types.h>
#include <core/list.h>
#include <system/timer.h>

/*
 * Deferred work executed from the scheduler task, never from interrupt
 * context.  Delayed work reuses the software timer for the wake-up.
 */

struct work;

typedef void (*work_fn_t)(struct work *work);

struct work {
    struct list_head node;
    work_fn_t fn;
    const char *name;
    u32 delay_ms;
    bool pending;
    struct timer timer;
};

struct workqueue {
    struct list_head work;
    const char *name;
};

void workqueue_init(struct workqueue *wq, const char *name);
int workqueue_queue(struct workqueue *wq, struct work *work, u32 delay_ms);
bool workqueue_cancel(struct work *work);
void workqueue_flush(struct workqueue *wq);

/* Called by the workqueue task. */
void workqueue_run(struct workqueue *wq);

/* Convenience: one system workqueue owned by the kernel. */
struct workqueue *system_workqueue(void);

#endif /* __SYSTEM_WORKQUEUE_H__ */
