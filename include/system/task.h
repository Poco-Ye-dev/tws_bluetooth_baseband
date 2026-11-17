#ifndef __SYSTEM_TASK_H__
#define __SYSTEM_TASK_H__

#include <core/types.h>
#include <core/compiler.h>
#include <core/list.h>

/*
 * Cooperative task scheduler.
 *
 * The firmware does not preempt: a task runs until it yields, sleeps or
 * blocks.  That matches the existing SDK behaviour and keeps the run-to-
 * completion audio callbacks cheap.  Interrupt handlers may post events and
 * wake tasks, but never switch context themselves.
 */

#define TASK_PRIO_IDLE    0
#define TASK_PRIO_LOW     1
#define TASK_PRIO_NORMAL  3
#define TASK_PRIO_HIGH    5
#define TASK_PRIO_REALTIME 7

#define TASK_NAME_MAX 12

struct task;

typedef void (*task_entry_t)(void *arg);int os_init(void);

struct task *task_create(const char *name, task_entry_t entry, void *arg,
                         u8 prio, u32 stack_size);
void task_destroy(struct task *task);
void task_start(void) __noreturn;
void task_yield(void);
void task_sleep(u32 ms);
void task_wakeup(struct task *task);
void task_exit(void) __noreturn;

/* Used by the blocking primitives in <system/sync.h>. */
void task_block(void);
void task_unblock(struct task *task);
void task_block_on(struct list_head *waiters);
struct task *task_wakeup_first(struct list_head *waiters);

struct task *task_self(void);
const char *task_name(struct task *task);
u32 task_count(void);

/* Called from the periodic tick (1 kHz). */
void task_tick(void);

#endif /* __SYSTEM_TASK_H__ */
