#include <bt/avctp.h>
#include <core/string.h>

int avrcp_init(struct avrcp *avrcp, void (*on_key)(u8, bool, void *), void *arg)
{
    if (!avrcp)
        return -1;
    memset(avrcp, 0, sizeof(*avrcp));
    avrcp->on_key = on_key;
    avrcp->arg = arg;
    return 0;
}

int avrcp_connect(struct avrcp *avrcp, u16 handle)
{
    if (!avrcp)
        return -1;
    avrcp->chan = l2cap_connect(AVCTP_PSM, handle, 0, NULL, NULL);
    return avrcp->chan ? 0 : -2;
}

static int send_frame(struct avrcp *avrcp, u8 ctype, const void *payload, u16 len)
{
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf || !avrcp->chan) {
        bt_buf_put(buf);
        return -1;
    }

    u8 hdr[3] = {
        (u8)((avrcp->transaction & 0x0F) << 4),
        0x11, 0x0E,                  /* AVCTP profile: AVRCP */
    };
    bt_buf_append(buf, hdr, sizeof(hdr));
    u8 avc[2] = { ctype, 0x00 };
    bt_buf_append(buf, avc, sizeof(avc));
    if (len && payload)
        bt_buf_append(buf, payload, len);

    int ret = l2cap_send(avrcp->chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    avrcp->transaction = (u8)((avrcp->transaction + 1) & 0x0F);
    return ret;
}

int avrcp_key(struct avrcp *avrcp, enum avc_opid opid, bool pressed)
{
    u8 payload[2] = { (u8)opid, (u8)(pressed ? 0 : 1) };
    return send_frame(avrcp, 0x00, payload, sizeof(payload));
}

int avrcp_unit_info(struct avrcp *avrcp)
{
    return send_frame(avrcp, 0x00, NULL, 0);
}

void avrcp_rx(struct avrcp *avrcp, struct bt_buf *buf)
{
    if (!avrcp || !buf || bt_buf_len(buf) < 6) {
        bt_buf_put(buf);
        return;
    }

    bt_buf_pull(buf, 3);             /* AVCTP header */
    bt_buf_pull(buf, 2);             /* AVC type + subunit */
    u8 opid = bt_buf_get_u8(buf);
    u8 state = bt_buf_get_u8(buf);

    if (avrcp->on_key)
        avrcp->on_key(opid, state == 0, avrcp->arg);
    bt_buf_put(buf);
}

void avrcp_close(struct avrcp *avrcp)
{
    if (avrcp && avrcp->chan)
        l2cap_disconnect(avrcp->chan);
}
