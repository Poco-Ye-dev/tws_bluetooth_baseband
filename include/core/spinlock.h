#ifndef __CORE_SPINLOCK_H__
#define __CORE_SPINLOCK_H__

#include <core/types.h>
#include <core/compiler.h>

/*
 * Spinlock plus the interrupt-state guard that must accompany it on a
 * single-core part: a lock taken in thread context has to mask the IRQ that
 * could otherwise re-enter the same critical section.
 */

typedef struct {
    volatile u32 locked;
} spinlock_t;

#define SPINLOCK_INIT { .locked = 0 }

/* Provided by the CPU port. */
unsigned long arch_irq_save(void);
void arch_irq_restore(unsigned long flags);

static inline void spin_lock(spinlock_t *lock)
{
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE))
        ;
}

static inline void spin_unlock(spinlock_t *lock)
{
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

static inline bool spin_trylock(spinlock_t *lock)
{
    return !__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE);
}

static inline bool spin_is_locked(const spinlock_t *lock)
{
    return !!__atomic_load_n(&lock->locked, __ATOMIC_RELAXED);
}

struct irq_flags {
    unsigned long flags;
};

#define spin_lock_irqsave(lock, f)     \
    do {                               \
        (f).flags = arch_irq_save();   \
        spin_lock(lock);               \
    } while (0)

#define spin_unlock_irqrestore(lock, f) \
    do {                               \
        spin_unlock(lock);             \
        arch_irq_restore((f).flags);   \
    } while (0)

#endif /* __CORE_SPINLOCK_H__ */
