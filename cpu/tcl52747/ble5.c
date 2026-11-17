#include <bt/le_hw.h>

/*
 * tcl52747 BLE5 port.  The BLE engine sits at 0x40080000; the port only
 * supplies the base address and the radio front end binding.
 */

#define BLE5_BASE 0x40080000u

static struct le_hw hw;

int ble5_port_init(void)
{
    if (le_hw_init(&hw, "tcl52747-ble5", (void *)BLE5_BASE, 32))
        return -1;
    return le_hw_enable(&hw, true);
}

struct le_hw *ble5_hw(void)
{
    return &hw;
}
