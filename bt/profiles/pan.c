#include <bt/pan.h>
#include <core/string.h>

#define BNEP_GENERAL_ETHERNET 0x00

int pan_init(struct pan *pan, const u8 local_addr[6],
             void (*on_frame)(const u8 *, u16, void *), void *arg)
{
    if (!pan)
        return -1;
    memset(pan, 0, sizeof(*pan));
    if (local_addr)
        memcpy(pan->local_addr, local_addr, 6);
    pan->on_frame = on_frame;
    pan->arg = arg;
    return 0;
}

int pan_connect(struct pan *pan, u16 handle)
{
    if (!pan)
        return -1;
    pan->chan = l2cap_connect(BNEP_PSM, handle, 0, NULL, NULL);
    return pan->chan ? 0 : -2;
}

int pan_send(struct pan *pan, const u8 *frame, u16 len)
{
    u8 pdu[2 + 1500];

    if (!pan || !pan->chan || !frame || len > 1500)
        return -1;

    pdu[0] = BNEP_GENERAL_ETHERNET;
    memcpy(&pdu[2], frame, len);
    pdu[1] = 0;
    return l2cap_send(pan->chan, pdu, (u16)(len + 2));
}

void pan_rx(struct pan *pan, struct bt_buf *buf)
{
    if (!pan || !buf || bt_buf_len(buf) < 1) {
        bt_buf_put(buf);
        return;
    }

    bt_buf_pull(buf, 1);
    u16 len = bt_buf_len(buf);
    if (pan->on_frame)
        pan->on_frame(buf->data, len, pan->arg);
    bt_buf_put(buf);
}

void pan_close(struct pan *pan)
{
    if (pan && pan->chan)
        l2cap_disconnect(pan->chan);
}
