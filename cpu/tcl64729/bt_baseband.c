#include <bt/edr_hw.h>
#include <bt/le_hw.h>

/*
 * tcl64729 classic baseband port: simulated register window at 0x40090000.
 * Real silicon uses a different base; the layout matches include/bt/edr_hw.h.
 */

#define BT_BASEBAND_BASE 0x40090000u

static struct edr_hw hw;

int bt_baseband_port_init(void)
{
    if (edr_hw_init(&hw, "tcl64729-bt", (void *)BT_BASEBAND_BASE, 32))
        return -1;

    edr_hw_rf_init(&hw, 4);
    edr_hw_txbuf_config(&hw, BT_BASEBAND_BASE + 0x800, 1024);
    edr_hw_rxbuf_config(&hw, BT_BASEBAND_BASE + 0xC00, 1024);
    edr_hw_set_window(&hw, 0x20, 0);
    edr_hw_set_slot_trigger(&hw, 1);
    edr_hw_packet_init(&hw);
    edr_hw_power(&hw, true);
    return 0;
}

struct edr_hw *bt_baseband_hw(void)
{
    return &hw;
}
