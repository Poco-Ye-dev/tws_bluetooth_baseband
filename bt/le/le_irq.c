#include <bt/le_irq.h>
#include <bt/le_hw.h>
#include <driver/irq.h>

/*
 * LE interrupt handling for all eight hardware instances.
 *
 * Each instance has its own IRQ line (anchor/rx/tx/dma/cte).  The handler
 * acknowledges the hardware and forwards to the link layer; it never parses
 * or copies PDU payload beyond reading the RX buffer.
 */

#define LE_IRQ_BASE_NUM 30
#define LE_IRQ_LINES    5

static struct le_irq_ops irq_ops;

void le_irq_register_ops(const struct le_irq_ops *ops)
{
    if (ops)
        irq_ops = *ops;
}

static struct le_hw *hw_of(void *arg)
{
    return le_hw_by_index((u8)(uptr)arg);
}

static void le_anchor_irq(void *arg)
{
    struct le_hw *hw = hw_of(arg);
    u32 status = le_hw_irq_status(hw);

    if (!(status & LE_IRQ_ANCHOR))
        return;
    le_hw_irq_ack(hw, LE_IRQ_ANCHOR);
    if (irq_ops.anchor)
        irq_ops.anchor((u16)(uptr)arg);
}

static void le_rx_irq(void *arg)
{
    struct le_hw *hw = hw_of(arg);
    u32 status = le_hw_irq_status(hw);
    u8 pdu[258];
    int len;

    if (!(status & LE_IRQ_RX_DONE))
        return;

    len = le_hw_rx_read(hw, pdu, sizeof(pdu));
    le_hw_irq_ack(hw, LE_IRQ_RX_DONE);

    if (irq_ops.rx_done && len > 0)
        irq_ops.rx_done((u16)(uptr)arg, pdu, (u16)len);
}

static void le_tx_irq(void *arg)
{
    struct le_hw *hw = hw_of(arg);
    u32 status = le_hw_irq_status(hw);

    if (!(status & LE_IRQ_TX_DONE))
        return;
    le_hw_irq_ack(hw, LE_IRQ_TX_DONE);
    if (irq_ops.tx_done)
        irq_ops.tx_done((u16)(uptr)arg);
}

static void le_dma_irq(void *arg)
{
    struct le_hw *hw = hw_of(arg);
    u32 status = le_hw_irq_status(hw);

    if (!(status & LE_IRQ_DMA))
        return;
    le_hw_irq_ack(hw, LE_IRQ_DMA);
    if (irq_ops.dma_done)
        irq_ops.dma_done();
}

static void le_cte_irq(void *arg)
{
    struct le_hw *hw = hw_of(arg);
    u32 status = le_hw_irq_status(hw);

    if (!(status & LE_IRQ_CTE))
        return;
    le_hw_irq_ack(hw, LE_IRQ_CTE);
    if (irq_ops.cte_done)
        irq_ops.cte_done();
}

int le_irq_init(void)
{
    static const struct {
        void (*fn)(void *);
        const char *name;
    } lines[LE_IRQ_LINES] = {
        { le_anchor_irq, "le-anchor" },
        { le_rx_irq,     "le-rx" },
        { le_tx_irq,     "le-tx" },
        { le_dma_irq,    "le-dma" },
        { le_cte_irq,    "le-cte" },
    };

    for (u8 i = 0; i < LE_HW_MAX; i++) {
        struct le_hw *hw = le_hw_by_index(i);
        if (!hw)
            continue;

        for (unsigned int j = 0; j < LE_IRQ_LINES; j++) {
            u32 irq = LE_IRQ_BASE_NUM + i * LE_IRQ_LINES + j;
            irq_register(irq, lines[j].fn, (void *)(uptr)i, lines[j].name);
            irq_enable(irq);
        }

        le_hw_irq_enable(hw, LE_IRQ_ANCHOR | LE_IRQ_RX_DONE |
                             LE_IRQ_TX_DONE | LE_IRQ_DMA | LE_IRQ_CTE);
    }
    return 0;
}
