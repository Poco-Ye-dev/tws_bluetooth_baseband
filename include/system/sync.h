#ifndef __SYSTEM_SYNC_H__
#define __SYSTEM_SYNC_H__

#include <core/types.h>
#include <core/list.h>
#include <core/atomic.h>

/*
 * Blocking primitives.  Both build on the scheduler: a waiter is moved off
 * the ready list and put back when the object is released.  They must not be
 * used from interrupt context.
 */

struct mutex {
    atomic_t owner;
    struct list_head waiters;
    const char *name;
};

void mutex_init(struct mutex *m, const char *name);
void mutex_lock(struct mutex *m);
bool mutex_trylock(struct mutex *m);
void mutex_unlock(struct mutex *m);
bool mutex_is_locked(struct mutex *m);

struct semaphore {
    atomic_t count;
    struct list_head waiters;
};

void sem_init(struct semaphore *sem, u32 count);
void sem_wait(struct semaphore *sem);
bool sem_trywait(struct semaphore *sem);
void sem_post(struct semaphore *sem);
u32 sem_value(struct semaphore *sem);

#endif /* __SYSTEM_SYNC_H__ */
