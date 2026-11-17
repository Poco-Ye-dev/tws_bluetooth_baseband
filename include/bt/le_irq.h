#ifndef __BT_LE_IRQ_H__
#define __BT_LE_IRQ_H__

#include <core/types.h>

/*
 * LE baseband interrupt callbacks.  The first argument is the HW instance
 * index (0..7), not the connection handle, so one handler set can serve all
 * eight hardware contexts.
 */

struct le_irq_ops {
    void (*anchor)(u16 hw_index);
    void (*rx_done)(u16 hw_index, const u8 *pdu, u16 len);
    void (*tx_done)(u16 hw_index);
    void (*dma_done)(void);
    void (*cte_done)(void);
};

int le_irq_init(void);
void le_irq_register_ops(const struct le_irq_ops *ops);

#endif /* __BT_LE_IRQ_H__ */
