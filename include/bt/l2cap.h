#ifndef __BT_L2CAP_H__
#define __BT_L2CAP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/buf.h>

/*
 * L2CAP: logical channels over ACL.
 *
 * A channel is a small explicit state machine.  Signaling runs on CID 1 and
 * is handled entirely inside the core; upper layers only see channel events.
 */

#define L2CAP_CID_SIGNALING 0x0001
#define L2CAP_CID_ATT       0x0004
#define L2CAP_CID_SMP       0x0006
#define L2CAP_CID_DYNAMIC   0x0040
#define L2CAP_DEFAULT_MTU   672

enum l2cap_state {
    L2CAP_CLOSED = 0,
    L2CAP_WAIT_CONNECT,
    L2CAP_WAIT_CONFIG,
    L2CAP_OPEN,
    L2CAP_WAIT_DISCONNECT,
};

struct l2cap_chan;

struct l2cap_ops {
    void (*connected)(struct l2cap_chan *chan);
    void (*disconnected)(struct l2cap_chan *chan, u8 reason);
    void (*data)(struct l2cap_chan *chan, struct bt_buf *buf);
    void (*mtu_changed)(struct l2cap_chan *chan, u16 mtu);
};

struct l2cap_chan {
    u16 scid;
    u16 dcid;
    u16 psm;
    u16 mtu;
    u16 handle;
    enum l2cap_state state;
    bool initiator;
    bool dynamic_cid;
    u8 config_state;
    const struct l2cap_ops *ops;
    void *arg;
    struct bt_buf *rx;
    struct l2cap_chan *next;
};

int l2cap_init(void);
int l2cap_register_psm(u16 psm, const struct l2cap_ops *ops, void *arg);
struct l2cap_chan *l2cap_connect(u16 psm, u16 handle, u16 mtu,
                                 const struct l2cap_ops *ops, void *arg);
int l2cap_send(struct l2cap_chan *chan, const void *data, u16 len);
int l2cap_disconnect(struct l2cap_chan *chan);
void l2cap_rx(u16 handle, struct bt_buf *buf);
struct l2cap_chan *l2cap_chan_by_cid(u16 cid);

/* Fixed CID channels (ATT, SMP) bypass the PSM machinery. */
int l2cap_register_fixed(u16 cid, const struct l2cap_ops *ops, void *arg);
struct l2cap_chan *l2cap_fixed_chan(u16 cid);

#endif /* __BT_L2CAP_H__ */
