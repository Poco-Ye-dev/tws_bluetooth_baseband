#include <system/task.h>
#include <system/timer.h>
#include <core/list.h>
#include <core/mem.h>
#include <core/string.h>
#include <core/printk.h>

/*
 * Cooperative scheduler.  The ready list is sorted by priority (highest
 * first); equal priorities rotate round robin because a yielded task is
 * appended behind its peers.
 *
 * Stack switching is delegated to the CPU port through arch_task_switch().
 * The generic code only decides *which* task runs next.
 */

enum task_state {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_BLOCKED,
    TASK_DEAD,
};

struct task {
    struct list_head node;
    char name[TASK_NAME_MAX];
    task_entry_t entry;
    void *arg;
    u8 prio;
    u8 state;
    u32 wake_ms;
    void *stack;
    u32 stack_size;
    void *sp;
};

#define MAX_TASKS 16

static struct task tasks[MAX_TASKS];
static u32 task_used;
static struct list_head ready_list;
static struct task *current;
static struct task idle_task;

__weak void arch_task_switch(struct task *prev, struct task *next)
{
    (void)prev;
    (void)next;
}

__weak void arch_idle(void)
{
}

static void ready_insert(struct task *task)
{
    struct list_head *pos;

    list_for_each(pos, &ready_list) {
        struct task *peer = list_entry(pos, struct task, node);
        if (peer->prio < task->prio)
            break;
    }
    list_add_tail(&task->node, pos);
}

static struct task *pick_next(void)
{
    if (list_empty(&ready_list))
        return &idle_task;
    return list_first_entry(&ready_list, struct task, node);
}

static void schedule(void)
{
    struct task *prev = current;
    struct task *next = pick_next();

    if (next == prev)
        return;

    if (prev && prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
        ready_insert(prev);
    }

    if (next != &idle_task)
        list_del_init(&next->node);
    next->state = TASK_RUNNING;
    current = next;

    arch_task_switch(prev, next);
}

int os_init(void)
{
    INIT_LIST_HEAD(&ready_list);
    current = NULL;
    task_used = 0;
    memset(tasks, 0, sizeof(tasks));

    idle_task.state = TASK_READY;
    idle_task.prio = TASK_PRIO_IDLE;
    strcpy(idle_task.name, "idle");
    return 0;
}

struct task *task_create(const char *name, task_entry_t entry, void *arg,
                         u8 prio, u32 stack_size)
{
    struct task *task;

    if (!entry || task_used >= MAX_TASKS)
        return NULL;

    task = &tasks[task_used++];
    memset(task, 0, sizeof(*task));
    strncpy(task->name, name ? name : "task", TASK_NAME_MAX - 1);
    task->entry = entry;
    task->arg = arg;
    task->prio = prio;
    task->state = TASK_READY;
    task->stack_size = stack_size;

    if (stack_size) {
        task->stack = kzalloc(stack_size);
        if (!task->stack) {
            task->state = TASK_DEAD;
            return NULL;
        }
    }

    list_add_tail(&task->node, &ready_list);
    return task;
}

void task_destroy(struct task *task)
{
    if (!task || task == &idle_task)
        return;

    list_del_init(&task->node);
    kfree(task->stack);
    task->stack = NULL;
    task->state = TASK_DEAD;
}

void task_start(void)
{
    if (!current) {
        current = pick_next();
        if (current != &idle_task)
            list_del_init(&current->node);
        current->state = TASK_RUNNING;
        arch_task_switch(NULL, current);
    }

    for (;;) {
        schedule();
        if (current == &idle_task)
            arch_idle();
    }
}

void task_yield(void)
{
    if (current == &idle_task)
        return;
    schedule();
}

void task_sleep(u32 ms)
{
    if (current == &idle_task)
        return;

    current->state = TASK_SLEEPING;
    current->wake_ms = system_ticks() + ms;
    schedule();
}

void task_block(void)
{
    if (current == &idle_task)
        return;

    current->state = TASK_BLOCKED;
    schedule();
}

void task_block_on(struct list_head *waiters)
{
    struct task *task = current;

    if (!task || task == &idle_task)
        return;

    list_add_tail(&task->node, waiters);
    task->state = TASK_BLOCKED;
    schedule();
}

struct task *task_wakeup_first(struct list_head *waiters)
{
    struct task *task;

    if (list_empty(waiters))
        return NULL;

    task = list_first_entry(waiters, struct task, node);
    list_del_init(&task->node);
    task_unblock(task);
    return task;
}

void task_unblock(struct task *task)
{
    if (!task || task == &idle_task || task->state == TASK_DEAD)
        return;

    task->state = TASK_READY;
    ready_insert(task);
}

void task_wakeup(struct task *task)
{
    task_unblock(task);
}

void task_exit(void)
{
    if (current == &idle_task)
        for (;;)
            ;

    current->state = TASK_DEAD;
    list_del_init(&current->node);
    schedule();
    for (;;)
        ;
}

struct task *task_self(void)
{
    return current;
}

const char *task_name(struct task *task)
{
    return task ? task->name : "(none)";
}

u32 task_count(void)
{
    u32 count = 0;

    for (u32 i = 0; i < task_used; i++) {
        if (tasks[i].state != TASK_DEAD)
            count++;
    }
    return count;
}

void task_tick(void)
{
    u32 now = system_ticks();

    for (u32 i = 0; i < task_used; i++) {
        struct task *task = &tasks[i];
        if (task->state == TASK_SLEEPING && (s32)(now - task->wake_ms) >= 0)
            task_unblock(task);
    }
}
