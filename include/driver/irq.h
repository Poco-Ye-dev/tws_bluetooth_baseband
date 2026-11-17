#ifndef __DRIVER_IRQ_H__
#define __DRIVER_IRQ_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Shared interrupt registry.  The CPU port acknowledges the source and then
 * calls irq_dispatch(); handlers registered here never touch the controller.
 */

typedef void (*irq_handler_t)(void *arg);

/* Implemented by the CPU port to mask/unmask a source. */
int arch_irq_control(u32 irq, bool enable);

int irq_register(u32 irq, irq_handler_t handler, void *arg, const char *name);
int irq_unregister(u32 irq);
int irq_enable(u32 irq);
int irq_disable(u32 irq);
void irq_dispatch(u32 irq);

/* Counts every dispatch; useful for latency diagnostics. */
u32 irq_count(u32 irq);
int irq_set_priority(u32 irq, u8 priority);
u8 irq_get_priority(u32 irq);
u32 irq_nested_max(void);

#endif /* __DRIVER_IRQ_H__ */
