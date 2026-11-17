#include <bt/edr_irq.h>
#include <bt/edr_hw.h>
#include <driver/irq.h>

#define EDR_IRQ_BASE_NUM 40
#define EDR_IRQ_LINES    5

static struct edr_irq_ops irq_ops;

void edr_irq_register_ops(const struct edr_irq_ops *ops)
{
    if (ops)
        irq_ops = *ops;
}

static struct edr_hw *hw_of(void *arg)
{
    return edr_hw_by_index((u8)(uptr)arg);
}

static void edr_slot_irq(void *arg)
{
    struct edr_hw *hw = hw_of(arg);
    u32 status = edr_hw_irq_status(hw);

    if (status & EDR_IRQ_SLOT) {
        edr_hw_irq_ack(hw, EDR_IRQ_SLOT);
        if (irq_ops.slot)
            irq_ops.slot((u16)(uptr)arg, edr_hw_clkn(hw));
    }
    if (status & EDR_IRQ_CONNECT) {
        edr_hw_irq_ack(hw, EDR_IRQ_CONNECT);
        if (irq_ops.connect)
            irq_ops.connect((u16)(uptr)arg, (u8)hw->config.role);
    }
    if (status & EDR_IRQ_ERROR) {
        edr_hw_irq_ack(hw, EDR_IRQ_ERROR);
        if (irq_ops.error)
            irq_ops.error((u16)(uptr)arg, 1);
    }
}

static void edr_rx_irq(void *arg)
{
    struct edr_hw *hw = hw_of(arg);
    u32 status = edr_hw_irq_status(hw);
    u8 data[1024];
    int len;

    if (!(status & EDR_IRQ_RX_DONE))
        return;

    len = edr_hw_rx_read(hw, data, sizeof(data));
    edr_hw_irq_ack(hw, EDR_IRQ_RX_DONE);
    if (irq_ops.rx_frame && len > 0)
        irq_ops.rx_frame((u16)(uptr)arg, data, (u16)len);
}

static void edr_tx_irq(void *arg)
{
    struct edr_hw *hw = hw_of(arg);
    u32 status = edr_hw_irq_status(hw);

    if (!(status & EDR_IRQ_TX_DONE))
        return;
    edr_hw_irq_ack(hw, EDR_IRQ_TX_DONE);
    if (irq_ops.tx_done)
        irq_ops.tx_done((u16)(uptr)arg);
}

int edr_irq_init(void)
{
    static const struct {
        void (*fn)(void *);
        const char *name;
    } lines[EDR_IRQ_LINES] = {
        { edr_slot_irq, "edr-slot" },
        { edr_rx_irq,   "edr-rx" },
        { edr_tx_irq,   "edr-tx" },
        { edr_slot_irq, "edr-err" },
        { edr_slot_irq, "edr-conn" },
    };

    for (u8 i = 0; i < EDR_HW_MAX; i++) {
        struct edr_hw *hw = edr_hw_by_index(i);
        if (!hw)
            continue;

        for (unsigned int j = 0; j < EDR_IRQ_LINES; j++) {
            u32 irq = EDR_IRQ_BASE_NUM + i * EDR_IRQ_LINES + j;
            irq_register(irq, lines[j].fn, (void *)(uptr)i, lines[j].name);
            irq_enable(irq);
        }

        edr_hw_irq_enable(hw, EDR_IRQ_RX_DONE | EDR_IRQ_TX_DONE |
                             EDR_IRQ_SLOT | EDR_IRQ_ERROR | EDR_IRQ_CONNECT);
    }
    return 0;
}
