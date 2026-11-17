#ifndef __BT_AVCTP_H__
#define __BT_AVCTP_H__

#include <core/types.h>
#include <bt/l2cap.h>
#include <bt/buf.h>

/*
 * AVRCP over AVCTP: passthrough commands and unit info.
 */

#define AVCTP_PSM 0x0017

enum avc_opid {
    AVC_OP_VOL_UP   = 0x41,
    AVC_OP_VOL_DOWN = 0x42,
    AVC_OP_PLAY     = 0x44,
    AVC_OP_STOP     = 0x45,
    AVC_OP_PAUSE    = 0x46,
    AVC_OP_NEXT     = 0x4B,
    AVC_OP_PREV     = 0x4C,
};

struct avrcp {
    struct l2cap_chan *chan;
    u8 transaction;
    void (*on_key)(u8 opid, bool pressed, void *arg);
    void *arg;
};

int avrcp_init(struct avrcp *avrcp, void (*on_key)(u8, bool, void *), void *arg);
int avrcp_connect(struct avrcp *avrcp, u16 handle);
int avrcp_key(struct avrcp *avrcp, enum avc_opid opid, bool pressed);
int avrcp_unit_info(struct avrcp *avrcp);
void avrcp_rx(struct avrcp *avrcp, struct bt_buf *buf);
void avrcp_close(struct avrcp *avrcp);

#endif /* __BT_AVCTP_H__ */
