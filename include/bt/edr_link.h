#ifndef __BT_EDR_LINK_H__
#define __BT_EDR_LINK_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/edr_hw.h>
#include <bt/le_ring.h>
#include <bt/le_timer.h>

/*
 * EDR link = one hardware frame engine + anchor timer + TX/RX rings.
 *
 * Mirrors the LE link architecture so both hardware domains are structurally
 * identical: timer opens the slot, IRQ fills the RX ring, the host drains it.
 */

#define EDR_LINK_MAX 8
#define EDR_LINK_RING_SIZE 512

struct edr_link {
    bool used;
    u16 handle;
    u8 hw_index;
    struct edr_hw *hw;
    struct le_ring tx;
    struct le_ring rx;
    u8 tx_storage[EDR_LINK_RING_SIZE];
    u8 rx_storage[EDR_LINK_RING_SIZE];
    struct le_timer *window_timer;
    u32 anchor;
    u16 interval_slots;
    bool window_open;
};

int edr_link_init(void);
struct edr_link *edr_link_open(u16 handle, const bt_addr *peer, u16 interval);
void edr_link_close(u16 handle);
int edr_link_send(u16 handle, const u8 *data, u16 len);
u16 edr_link_read(u16 handle, void *data, u16 cap);
void edr_link_tick(u32 slot);
struct edr_link *edr_link_by_hw(u8 hw_index);

#endif /* __BT_EDR_LINK_H__ */
