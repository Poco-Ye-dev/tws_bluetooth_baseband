#ifndef __BT_EDR_IRQ_H__
#define __BT_EDR_IRQ_H__

#include <core/types.h>

/*
 * EDR interrupt callbacks.  The first argument is the hardware instance
 * index (0..7) so one handler set serves all eight frame engines.
 */

struct edr_irq_ops {
    void (*slot)(u16 hw_index, u32 slot);
    void (*rx_frame)(u16 hw_index, const u8 *data, u16 len);
    void (*tx_done)(u16 hw_index);
    void (*connect)(u16 hw_index, u8 role);
    void (*error)(u16 hw_index, u8 code);
};

int edr_irq_init(void);
void edr_irq_register_ops(const struct edr_irq_ops *ops);

#endif /* __BT_EDR_IRQ_H__ */
