#ifndef __BT_LE_LINK_H__
#define __BT_LE_LINK_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/le_hw.h>
#include <bt/le_ll.h>
#include <bt/le_ring.h>
#include <bt/le_timer.h>

/*
 * One LE link = one HW instance + one anchor timer + TX/RX rings + LL state.
 *
 * TX ring: host -> link (le_link_send)
 * RX ring: IRQ   -> host (le_link_read)
 * The timer opens the receiver window at the anchor; the IRQ drains the HW
 * buffer into the RX ring.
 */

#define LE_LINK_MAX 8
#define LE_LINK_RING_SIZE 512

struct le_link {
    bool used;
    u16 handle;
    u8 hw_index;
    struct le_hw *hw;
    struct le_ll_conn *conn;
    struct le_ring tx;
    struct le_ring rx;
    u8 tx_storage[LE_LINK_RING_SIZE];
    u8 rx_storage[LE_LINK_RING_SIZE];
    struct le_timer *window_timer;
    u32 anchor;
    u16 interval_slots;
    u16 winsize;
    u8 widen;
    bool window_open;
};

int le_link_init(void);

/* Independent LE stack entry: owns its own tick, does not call EDR. */
int le_stack_init(void);
void le_stack_tick(u32 slot);
struct le_link *le_link_open(u16 handle, const bt_addr *peer,
                             u16 interval, u16 latency, u16 timeout);
void le_link_close(u16 handle);
int le_link_send(u16 handle, const u8 *data, u16 len);
u16 le_link_read(u16 handle, void *data, u16 cap);
void le_link_tick(u32 slot);
struct le_link *le_link_by_hw(u8 hw_index);

#endif /* __BT_LE_LINK_H__ */
