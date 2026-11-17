#ifndef __BT_RFCOMM_H__
#define __BT_RFCOMM_H__

#include <core/types.h>
#include <bt/l2cap.h>
#include <bt/buf.h>

/*
 * RFCOMM: serial port emulation over L2CAP.
 *
 * One multiplexer per connection carries up to 31 data link connections.
 * Frame assembly is separate from the per-DLC state machine so credit based
 * flow control stays local to the channel.
 */

#define RFCOMM_PSM 0x0003
#define RFCOMM_MAX_DLC 8
#define RFCOMM_DEFAULT_MTU 127

enum rfcomm_state {
    RFCOMM_CLOSED = 0,
    RFCOMM_WAIT_UA,
    RFCOMM_OPEN,
    RFCOMM_WAIT_DISC,
};

struct rfcomm_dlc;

struct rfcomm_ops {
    void (*connected)(struct rfcomm_dlc *dlc);
    void (*disconnected)(struct rfcomm_dlc *dlc);
    void (*data)(struct rfcomm_dlc *dlc, struct bt_buf *buf);
};

struct rfcomm_dlc {
    u8 dlci;
    u16 mtu;
    u8 credits;
    enum rfcomm_state state;
    struct l2cap_chan *chan;
    const struct rfcomm_ops *ops;
    void *arg;
    struct rfcomm_dlc *next;
};

struct rfcomm_mux {
    struct l2cap_chan *chan;
    struct rfcomm_dlc *dlc[RFCOMM_MAX_DLC];
    u8 dlc_count;
    u8 mtu;
};

int rfcomm_mux_init(struct rfcomm_mux *mux);
int rfcomm_mux_attach(struct rfcomm_mux *mux, struct l2cap_chan *chan);
struct rfcomm_dlc *rfcomm_connect(struct rfcomm_mux *mux, u8 dlci,
                                  const struct rfcomm_ops *ops, void *arg);
int rfcomm_send(struct rfcomm_dlc *dlc, const void *data, u16 len);
int rfcomm_disconnect(struct rfcomm_dlc *dlc);
void rfcomm_mux_rx(struct rfcomm_mux *mux, struct bt_buf *buf);

#endif /* __BT_RFCOMM_H__ */
