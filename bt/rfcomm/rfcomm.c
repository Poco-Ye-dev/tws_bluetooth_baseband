#include <bt/rfcomm.h>
#include <core/string.h>
#include <core/mem.h>

#define RFCOMM_SABM 0x3F
#define RFCOMM_UA   0x73
#define RFCOMM_DISC 0x53
#define RFCOMM_UIH  0xEF

int rfcomm_mux_init(struct rfcomm_mux *mux)
{
    if (!mux)
        return -1;
    memset(mux, 0, sizeof(*mux));
    mux->mtu = RFCOMM_DEFAULT_MTU;
    return 0;
}

int rfcomm_mux_attach(struct rfcomm_mux *mux, struct l2cap_chan *chan)
{
    if (!mux)
        return -1;
    mux->chan = chan;
    return 0;
}

static int rfcomm_frame(struct rfcomm_mux *mux, u8 dlci, u8 control,
                        const void *data, u16 len)
{
    struct bt_buf *buf;
    u8 hdr[4];

    if (!mux || !mux->chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    hdr[0] = (u8)((dlci << 2) | 0x03);
    hdr[1] = control;
    hdr[2] = (u8)((len << 1) & 0xFF);
    hdr[3] = (u8)(((len << 1) >> 8) & 0xFF);
    bt_buf_append(buf, hdr, sizeof(hdr));
    if (len && data)
        bt_buf_append(buf, data, len);

    int ret = l2cap_send(mux->chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    return ret;
}

struct rfcomm_dlc *rfcomm_connect(struct rfcomm_mux *mux, u8 dlci,
                                  const struct rfcomm_ops *ops, void *arg)
{
    struct rfcomm_dlc *dlc;

    if (!mux || mux->dlc_count >= RFCOMM_MAX_DLC)
        return NULL;

    dlc = kzalloc(sizeof(*dlc));
    if (!dlc)
        return NULL;

    dlc->dlci = dlci;
    dlc->mtu = mux->mtu;
    dlc->credits = 7;
    dlc->state = RFCOMM_WAIT_UA;
    dlc->chan = mux->chan;
    dlc->ops = ops;
    dlc->arg = arg;

    mux->dlc[mux->dlc_count++] = dlc;
    if (rfcomm_frame(mux, dlci, RFCOMM_SABM, NULL, 0)) {
        mux->dlc[--mux->dlc_count] = NULL;
        kfree(dlc);
        return NULL;
    }
    return dlc;
}

int rfcomm_send(struct rfcomm_dlc *dlc, const void *data, u16 len)
{
    if (!dlc || dlc->state != RFCOMM_OPEN || len > dlc->mtu)
        return -1;
    return rfcomm_frame(NULL, dlc->dlci, RFCOMM_UIH, data, len);
}

int rfcomm_disconnect(struct rfcomm_dlc *dlc)
{
    if (!dlc)
        return -1;
    dlc->state = RFCOMM_WAIT_DISC;
    return rfcomm_frame(NULL, dlc->dlci, RFCOMM_DISC, NULL, 0);
}

void rfcomm_mux_rx(struct rfcomm_mux *mux, struct bt_buf *buf)
{
    struct rfcomm_dlc *dlc = NULL;

    if (!mux || !buf)
        return;

    u8 addr = bt_buf_get_u8(buf);
    u8 control = bt_buf_get_u8(buf);
    u16 len = (u16)(bt_buf_get_le16(buf) >> 1);
    u8 dlci = (u8)((addr >> 2) & 0x3F);
    const u8 *payload = bt_buf_pull(buf, len);

    for (unsigned int i = 0; i < mux->dlc_count; i++) {
        if (mux->dlc[i]->dlci == dlci) {
            dlc = mux->dlc[i];
            break;
        }
    }

    if (control == RFCOMM_SABM) {
        rfcomm_frame(mux, dlci, RFCOMM_UA, NULL, 0);
        if (dlc && dlc->state == RFCOMM_WAIT_UA) {
            dlc->state = RFCOMM_OPEN;
            if (dlc->ops && dlc->ops->connected)
                dlc->ops->connected(dlc);
        }
    } else if (control == RFCOMM_UA) {
        if (dlc && dlc->state == RFCOMM_WAIT_UA) {
            dlc->state = RFCOMM_OPEN;
            if (dlc->ops && dlc->ops->connected)
                dlc->ops->connected(dlc);
        }
    } else if (control == RFCOMM_DISC) {
        rfcomm_frame(mux, dlci, RFCOMM_UA, NULL, 0);
        if (dlc && dlc->ops && dlc->ops->disconnected)
            dlc->ops->disconnected(dlc);
        if (dlc)
            dlc->state = RFCOMM_CLOSED;
    } else if (control == RFCOMM_UIH && dlc && payload) {
        if (dlc->ops && dlc->ops->data) {
            struct bt_buf *data = bt_buf_alloc(0);
            if (data) {
                bt_buf_append(data, payload, len);
                dlc->ops->data(dlc, data);
                bt_buf_put(data);
            }
        }
    }

    bt_buf_put(buf);
}
