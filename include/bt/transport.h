#ifndef __BT_TRANSPORT_H__
#define __BT_TRANSPORT_H__

#include <core/types.h>

/*
 * HCI transport contract.  A transport owns the physical link and calls
 * hci_rx() for every received packet.  Flow control is push based: the
 * transport returns BT_ERR_BUSY when its FIFO is full and the HCI core will
 * retry from its queue.
 */

struct bt_transport {
    const char *name;
    int (*open)(void);
    int (*close)(void);
    int (*send)(const u8 *data, u16 len);
    u16 (*read)(u8 *data, u16 len);
};

int bt_transport_register(const struct bt_transport *transport);
const char *bt_transport_name(void);

#endif /* __BT_TRANSPORT_H__ */
