#include <core/spinlock.h>
#include <core/compiler.h>
#include <driver/irq.h>
#include <driver/uart.h>
#include <core/printk.h>

/*
 * CPU port for the tcl family.
 *
 * The interrupt mask is kept in RAM and applied by the assembly entry code,
 * which keeps the C helpers branch free.  A real port would poke the core's
 * status register here instead.
 */

static unsigned long irq_state;

unsigned long arch_irq_save(void)
{
    unsigned long flags = irq_state;
    irq_state = 1;
    barrier();
    return flags;
}

void arch_irq_restore(unsigned long flags)
{
    barrier();
    irq_state = flags;
}

int arch_irq_control(u32 irq, bool enable)
{
    (void)irq;
    if (enable)
        irq_state = 0;
    else
        irq_state = 1;
    return 0;
}

void arch_idle(void)
{
    __asm__ __volatile__("nop");
}

void arch_reboot(void)
{
    for (;;)
        ;
}

u32 arch_cycles(void)
{
    return 0;
}

/* Force select()/poll() style console output through UART0. */
void platform_putc(char c)
{
    uart_putc(0, c);
}
