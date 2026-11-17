#include <bt/avdtp.h>
#include <core/string.h>

static void sig_send(struct a2dp *a2dp, u8 msg, const void *data, u16 len)
{
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return;
    u8 hdr[2] = { (u8)((a2dp->transaction << 4) | msg), 0 };
    bt_buf_append(buf, hdr, 2);
    if (len && data)
        bt_buf_append(buf, data, len);
    if (a2dp->sig)
        l2cap_send(a2dp->sig, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    a2dp->transaction = (u8)((a2dp->transaction + 1) & 0x0F);
}

int a2dp_init(struct a2dp *a2dp, void (*on_stream)(bool, void *), void *arg)
{
    if (!a2dp)
        return -1;
    memset(a2dp, 0, sizeof(*a2dp));
    a2dp->on_stream = on_stream;
    a2dp->arg = arg;
    a2dp->seid = 1;
    a2dp->codec = 0;                 /* SBC */
    return 0;
}

int a2dp_connect(struct a2dp *a2dp, u16 handle)
{
    if (!a2dp)
        return -1;
    a2dp->handle = handle;
    a2dp->sig = l2cap_connect(AVDTP_PSM, handle, 0, NULL, NULL);
    if (!a2dp->sig)
        return -2;
    sig_send(a2dp, AVDTP_DISCOVER, NULL, 0);
    return 0;
}

static void discover_reply(struct a2dp *a2dp, u8 transaction)
{
    u8 seps[] = {
        (u8)((1 << 2) | 0x02), 0x00, 0x07, /* SBC sink */
        (u8)((2 << 2) | 0x02), 0x00, 0x07, /* SBC source */
    };
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return;
    u8 hdr[2] = { (u8)((transaction << 4) | 0x02), 0 };
    bt_buf_append(buf, hdr, 2);
    bt_buf_append(buf, seps, sizeof(seps));
    if (a2dp->sig)
        l2cap_send(a2dp->sig, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

static void caps_reply(struct a2dp *a2dp, u8 transaction)
{
    u8 caps[] = {
        0x01, 0x00, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00,
    };
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return;
    u8 hdr[2] = { (u8)((transaction << 4) | 0x02), 0 };
    bt_buf_append(buf, hdr, 2);
    bt_buf_append(buf, caps, sizeof(caps));
    if (a2dp->sig)
        l2cap_send(a2dp->sig, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

int a2dp_start(struct a2dp *a2dp)
{
    if (!a2dp)
        return -1;
    sig_send(a2dp, AVDTP_START, NULL, 0);
    a2dp->state = AVDTP_ST_STREAMING;
    if (a2dp->on_stream)
        a2dp->on_stream(true, a2dp->arg);
    return 0;
}

int a2dp_stop(struct a2dp *a2dp)
{
    if (!a2dp)
        return -1;
    sig_send(a2dp, AVDTP_SUSPEND, NULL, 0);
    a2dp->state = AVDTP_ST_OPEN;
    if (a2dp->on_stream)
        a2dp->on_stream(false, a2dp->arg);
    return 0;
}

int a2dp_send_media(struct a2dp *a2dp, const void *data, u16 len)
{
    if (!a2dp || !a2dp->media || a2dp->state != AVDTP_ST_STREAMING)
        return -1;

    int ret = l2cap_send(a2dp->media, data, len);
    if (!ret && a2dp->relay)
        a2dp->relay(data, len, 0, a2dp->relay_arg);
    return ret;
}

void a2dp_set_relay(struct a2dp *a2dp,
                    void (*relay)(const u8 *, u16, u32, void *), void *arg)
{
    if (!a2dp)
        return;
    a2dp->relay = relay;
    a2dp->relay_arg = arg;
}

void a2dp_rx(struct a2dp *a2dp, struct bt_buf *buf)
{
    if (!a2dp || !buf || bt_buf_len(buf) < 2) {
        bt_buf_put(buf);
        return;
    }

    u8 hdr = bt_buf_get_u8(buf);
    u8 msg = hdr & 0x0F;
    u8 transaction = (u8)(hdr >> 4);

    switch (msg) {
    case AVDTP_DISCOVER:
        discover_reply(a2dp, transaction);
        break;
    case AVDTP_GET_CAPABILITIES:
        caps_reply(a2dp, transaction);
        break;
    case AVDTP_SET_CONFIGURATION:
        a2dp->state = AVDTP_ST_CONFIGURED;
        sig_send(a2dp, 0x02, NULL, 0);
        break;
    case AVDTP_OPEN:
        if (!a2dp->media)
            a2dp->media = l2cap_connect(AVDTP_PSM, a2dp->handle, 0, NULL, NULL);
        a2dp->state = AVDTP_ST_OPEN;
        sig_send(a2dp, 0x02, NULL, 0);
        break;
    case AVDTP_START:
        a2dp->state = AVDTP_ST_STREAMING;
        sig_send(a2dp, 0x02, NULL, 0);
        if (a2dp->on_stream)
            a2dp->on_stream(true, a2dp->arg);
        break;
    case AVDTP_SUSPEND:
    case AVDTP_CLOSE:
        a2dp->state = AVDTP_ST_OPEN;
        sig_send(a2dp, 0x02, NULL, 0);
        if (a2dp->on_stream)
            a2dp->on_stream(false, a2dp->arg);
        break;
    default:
        break;
    }

    bt_buf_put(buf);
}

void a2dp_close(struct a2dp *a2dp)
{
    if (!a2dp)
        return;
    if (a2dp->media)
        l2cap_disconnect(a2dp->media);
    if (a2dp->sig)
        l2cap_disconnect(a2dp->sig);
    a2dp->state = AVDTP_ST_IDLE;
}
