#include <driver/irq.h>
#include <core/string.h>

/*
 * Interrupt handler table.  IRQ numbers are dense and small on this class of
 * part, so a flat array is both simpler and faster than a search tree.
 */

#define IRQ_MAX 64

struct irq_entry {
    irq_handler_t handler;
    void *arg;
    const char *name;
    u32 count;
};

static struct irq_entry entries[IRQ_MAX];
static u8 priorities[IRQ_MAX];
static u32 nested_depth;
static u32 nested_peak;

int irq_register(u32 irq, irq_handler_t handler, void *arg, const char *name)
{
    if (irq >= IRQ_MAX || !handler)
        return -EINVAL;

    entries[irq].handler = handler;
    entries[irq].arg = arg;
    entries[irq].name = name;
    entries[irq].count = 0;
    return 0;
}

int irq_unregister(u32 irq)
{
    if (irq >= IRQ_MAX)
        return -EINVAL;

    entries[irq].handler = NULL;
    entries[irq].arg = NULL;
    return 0;
}

int irq_enable(u32 irq)
{
    if (irq >= IRQ_MAX || !entries[irq].handler)
        return -EINVAL;
    return arch_irq_control(irq, true);
}

int irq_disable(u32 irq)
{
    if (irq >= IRQ_MAX)
        return -EINVAL;
    return arch_irq_control(irq, false);
}

void irq_dispatch(u32 irq)
{
    if (irq >= IRQ_MAX || !entries[irq].handler)
        return;

    entries[irq].count++;
    nested_depth++;
    if (nested_depth > nested_peak)
        nested_peak = nested_depth;
    entries[irq].handler(entries[irq].arg);
    nested_depth--;
}

u32 irq_count(u32 irq)
{
    return irq < IRQ_MAX ? entries[irq].count : 0;
}

int irq_set_priority(u32 irq, u8 priority)
{
    if (irq >= IRQ_MAX || priority > 15)
        return -EINVAL;
    priorities[irq] = priority;
    return 0;
}

u8 irq_get_priority(u32 irq)
{
    return irq < IRQ_MAX ? priorities[irq] : 0;
}

u32 irq_nested_max(void)
{
    return nested_peak;
}
