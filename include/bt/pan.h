#ifndef __BT_PAN_H__
#define __BT_PAN_H__

#include <core/types.h>
#include <bt/l2cap.h>

/*
 * PAN user: BNEP encapsulation of Ethernet frames over L2CAP.
 */

#define BNEP_PSM 0x000F

struct pan {
    struct l2cap_chan *chan;
    u8 local_addr[6];
    void (*on_frame)(const u8 *frame, u16 len, void *arg);
    void *arg;
};

int pan_init(struct pan *pan, const u8 local_addr[6],
             void (*on_frame)(const u8 *, u16, void *), void *arg);
int pan_connect(struct pan *pan, u16 handle);
int pan_send(struct pan *pan, const u8 *frame, u16 len);
void pan_rx(struct pan *pan, struct bt_buf *buf);
void pan_close(struct pan *pan);

#endif /* __BT_PAN_H__ */
