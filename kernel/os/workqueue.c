#include <system/workqueue.h>
#include <system/task.h>
#include <core/spinlock.h>
#include <core/string.h>
#include <core/mem.h>

/*
 * Workqueue implementation.
 *
 * Delayed items arm a software timer; when it fires the item is moved onto
 * the same list as immediate work.  A single lock protects the list because
 * callers may queue from a timer callback.
 */

static struct workqueue system_wq;
static spinlock_t wq_lock = SPINLOCK_INIT;
static bool system_ready;

static void delayed_enqueue(void *arg)
{
    struct work *work = arg;
    struct workqueue *wq = system_workqueue();
    struct irq_flags flags;

    spin_lock_irqsave(&wq_lock, flags);
    if (work->pending && work->node.next == NULL)
        list_add_tail(&work->node, &wq->work);
    spin_unlock_irqrestore(&wq_lock, flags);
}

void workqueue_init(struct workqueue *wq, const char *name)
{
    INIT_LIST_HEAD(&wq->work);
    wq->name = name;
}

int workqueue_queue(struct workqueue *wq, struct work *work, u32 delay_ms)
{
    struct irq_flags flags;

    if (!wq || !work || !work->fn || work->pending)
        return -1;

    work->pending = true;

    if (delay_ms) {
        timer_init(&work->timer, work->name, delayed_enqueue, work);
        work->node.next = NULL;
        timer_start(&work->timer, delay_ms, false);
        return 0;
    }

    spin_lock_irqsave(&wq_lock, flags);
    list_add_tail(&work->node, &wq->work);
    spin_unlock_irqrestore(&wq_lock, flags);
    return 0;
}

bool workqueue_cancel(struct work *work)
{
    struct irq_flags flags;

    if (!work || !work->pending)
        return false;

    spin_lock_irqsave(&wq_lock, flags);
    timer_stop(&work->timer);
    if (work->node.next) {
        list_del_init(&work->node);
    }
    work->pending = false;
    spin_unlock_irqrestore(&wq_lock, flags);
    return true;
}

void workqueue_run(struct workqueue *wq)
{
    struct work *work;
    struct irq_flags flags;

    for (;;) {
        spin_lock_irqsave(&wq_lock, flags);
        if (list_empty(&wq->work)) {
            spin_unlock_irqrestore(&wq_lock, flags);
            return;
        }
        work = list_first_entry(&wq->work, struct work, node);
        list_del_init(&work->node);
        work->pending = false;
        spin_unlock_irqrestore(&wq_lock, flags);

        work->fn(work);
    }
}

void workqueue_flush(struct workqueue *wq)
{
    workqueue_run(wq);
}

struct workqueue *system_workqueue(void)
{
    if (!system_ready) {
        workqueue_init(&system_wq, "system");
        system_ready = true;
    }
    return &system_wq;
}
