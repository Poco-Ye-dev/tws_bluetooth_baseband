#ifndef __CPU_TCL_REGS_H__
#define __CPU_TCL_REGS_H__

#include <core/types.h>

/*
 * Register map common to the tcl family.  Values are byte offsets from the
 * peripheral base; the port files add the base address for their part.
 */

#define REG32(base, off) (*(volatile u32 *)((u8 *)(base) + (off)))

struct tcl_uart_regs {
    u32 data;
    u32 status;
    u32 baud;
    u32 control;
};

struct tcl_gpio_regs {
    u32 dir;
    u32 out;
    u32 in;
    u32 pull;
    u32 irq_en;
    u32 irq_status;
};

struct tcl_timer_regs {
    u32 load;
    u32 value;
    u32 control;
    u32 int_status;
};

#define TCL_GPIO_BASE   0x40010000u
#define TCL_UART0_BASE  0x40020000u
#define TCL_UART1_BASE  0x40021000u
#define TCL_I2C0_BASE   0x40030000u
#define TCL_SPI0_BASE   0x40040000u
#define TCL_I2S0_BASE   0x40050000u
#define TCL_TIMER_BASE  0x40060000u
#define TCL_IRQ_BASE    0x40070000u

#define TCL_IRQ_REG(off) REG32(TCL_IRQ_BASE, off)
#define TCL_IRQ_ENABLE   TCL_IRQ_REG(0x00)
#define TCL_IRQ_PENDING  TCL_IRQ_REG(0x04)
#define TCL_IRQ_ACK      TCL_IRQ_REG(0x08)

#endif /* __CPU_TCL_REGS_H__ */
