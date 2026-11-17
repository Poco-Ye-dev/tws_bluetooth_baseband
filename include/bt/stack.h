#ifndef __BT_STACK_H__
#define __BT_STACK_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/hci.h>
#include <bt/transport.h>

/*
 * Stack lifecycle and event bus.
 *
 * Subsystems never call each other's init; they subscribe to bus events and
 * the stack performs the ordered bring-up.  This keeps board files free of
 * knowledge about layer ordering.
 */

enum bt_event {
    BT_EVT_READY = 0,
    BT_EVT_CONNECTED,
    BT_EVT_DISCONNECTED,
    BT_EVT_PAIRING,
    BT_EVT_PAIRED,
    BT_EVT_ADV_REPORT,
    BT_EVT_ERROR,
};

typedef void (*bt_event_cb_t)(enum bt_event event, const void *data, void *arg);

struct bt_stack {
    const struct bt_transport *transport;
    bool ready;
    bt_addr local_addr;
    u8 subscribers;
};

int bt_stack_init(const struct bt_transport *transport);
int bt_stack_start(void);
void bt_stack_stop(void);
void bt_stack_poll(void);
bool bt_stack_ready(void);
const bt_addr *bt_stack_local_addr(void);

int bt_subscribe(bt_event_cb_t cb, void *arg);
void bt_publish(enum bt_event event, const void *data);

#endif /* __BT_STACK_H__ */
