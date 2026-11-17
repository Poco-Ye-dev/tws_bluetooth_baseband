#include <system/sync.h>
#include <system/task.h>
#include <core/errno.h>

void mutex_init(struct mutex *m, const char *name)
{
    atomic_set(&m->owner, 0);
    INIT_LIST_HEAD(&m->waiters);
    m->name = name;
}

void mutex_lock(struct mutex *m)
{
    if (atomic_cmpxchg(&m->owner, 0, 1) != 0) {
        task_block_on(&m->waiters);
        atomic_set(&m->owner, 1);
    }
}

bool mutex_trylock(struct mutex *m)
{
    return atomic_cmpxchg(&m->owner, 0, 1) == 0;
}

void mutex_unlock(struct mutex *m)
{
    if (!task_wakeup_first(&m->waiters))
        atomic_set(&m->owner, 0);
}

bool mutex_is_locked(struct mutex *m)
{
    return atomic_read(&m->owner) != 0;
}

void sem_init(struct semaphore *sem, u32 count)
{
    atomic_set(&sem->count, (s32)count);
    INIT_LIST_HEAD(&sem->waiters);
}

void sem_wait(struct semaphore *sem)
{
    for (;;) {
        if (sem_trywait(sem))
            return;
        task_block_on(&sem->waiters);
    }
}

bool sem_trywait(struct semaphore *sem)
{
    s32 value = atomic_read(&sem->count);

    while (value > 0) {
        if (atomic_cmpxchg(&sem->count, value, value - 1) == value)
            return true;
        value = atomic_read(&sem->count);
    }
    return false;
}

void sem_post(struct semaphore *sem)
{
    if (!task_wakeup_first(&sem->waiters))
        atomic_add(&sem->count, 1);
}

u32 sem_value(struct semaphore *sem)
{
    s32 value = atomic_read(&sem->count);
    return value > 0 ? (u32)value : 0;
}
