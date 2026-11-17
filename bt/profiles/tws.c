#include <bt/tws.h>
#include <bt/hci.h>
#include <bt/lmp.h>
#include <core/string.h>
#include <bt/avdtp.h>

/*
 * TWS v3 wire format: type(1) seq(2) len(2) payload.  Every message is
 * sequenced so the secondary can drop duplicates after a retransmission.
 */

#define TWS_HDR_LEN 5

static void hdr_write(u8 *hdr, u8 type, u16 seq, u16 len)
{
    hdr[0] = type;
    hdr[1] = (u8)(seq & 0xFF);
    hdr[2] = (u8)(seq >> 8);
    hdr[3] = (u8)(len & 0xFF);
    hdr[4] = (u8)(len >> 8);
}

static void hdr_read(const u8 *hdr, u8 *type, u16 *seq, u16 *len)
{
    *type = hdr[0];
    *seq = (u16)(hdr[1] | (hdr[2] << 8));
    *len = (u16)(hdr[3] | (hdr[4] << 8));
}

int tws_init(struct tws *tws, enum tws_role role,
             const struct tws_callbacks *cb, void *arg)
{
    if (!tws)
        return -1;

    memset(tws, 0, sizeof(*tws));
    tws->role = role;
    tws->sync_state = TWS_SYNC_IDLE;
    if (cb)
        tws->cb = *cb;
    tws->arg = arg;
    return 0;
}

int tws_connect(struct tws *tws, const bt_addr *peer, u16 handle)
{
    if (!tws || !peer)
        return -1;

    tws->peer = *peer;
    tws->handle = handle;
    tws->chan = l2cap_connect(TWS_PSM, handle, 0, NULL, NULL);
    if (!tws->chan)
        return -2;
    tws->sync_state = TWS_SYNC_WAIT_HELLO;
    if (tws->cb.on_link)
        tws->cb.on_link(true, tws->arg);
    return 0;
}

int tws_send(struct tws *tws, u8 type, const void *data, u16 len)
{
    struct bt_buf *buf;
    u8 hdr[TWS_HDR_LEN];

    if (!tws || !tws->chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    hdr_write(hdr, type, tws->seq++, len);
    bt_buf_append(buf, hdr, sizeof(hdr));
    if (len && data)
        bt_buf_append(buf, data, len);

    int ret = l2cap_send(tws->chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    if (!ret)
        tws->stats.tx_msgs++;
    return ret;
}

int tws_hello(struct tws *tws, const bt_addr *phone, u8 features)
{
    u8 payload[8];

    if (!tws)
        return -1;
    if (phone)
        tws->phone = *phone;

    payload[0] = (u8)tws->role;
    payload[1] = features;
    if (phone)
        memcpy(&payload[2], phone->b, 6);
    else
        memset(&payload[2], 0, 6);
    return tws_send(tws, TWS_MSG_HELLO, payload, sizeof(payload));
}

int tws_sync(struct tws *tws, u32 clock_offset, u32 anchor)
{
    u8 payload[8];

    if (!tws)
        return -1;

    tws->clock_offset = clock_offset;
    tws->anchor = anchor;
    payload[0] = (u8)(clock_offset & 0xFF);
    payload[1] = (u8)(clock_offset >> 8);
    payload[2] = (u8)(clock_offset >> 16);
    payload[3] = (u8)(clock_offset >> 24);
    payload[4] = (u8)(anchor & 0xFF);
    payload[5] = (u8)(anchor >> 8);
    payload[6] = (u8)(anchor >> 16);
    payload[7] = (u8)(anchor >> 24);
    return tws_send(tws, TWS_MSG_SYNC, payload, sizeof(payload));
}

int tws_relay_media(struct tws *tws, const void *data, u16 len, u32 timestamp)
{
    u8 payload[8 + TWS_MEDIA_MAX];

    if (!tws || tws->role != TWS_ROLE_PRIMARY)
        return -1;
    if (!data || len > TWS_MEDIA_MAX)
        return -2;

    payload[0] = (u8)(tws->media_seq & 0xFF);
    payload[1] = (u8)(tws->media_seq >> 8);
    payload[2] = (u8)(timestamp & 0xFF);
    payload[3] = (u8)(timestamp >> 8);
    payload[4] = (u8)(timestamp >> 16);
    payload[5] = (u8)(timestamp >> 24);
    memcpy(&payload[6], data, len);

    int ret = tws_send(tws, TWS_MSG_MEDIA, payload, (u16)(6 + len));
    if (!ret) {
        tws->media_seq++;
        tws->stats.relayed++;
    }
    return ret;
}

int tws_relay_media_up(struct tws *tws, const void *data, u16 len, u32 timestamp)
{
    u8 payload[8 + TWS_MEDIA_MAX];

    if (!tws || tws->role != TWS_ROLE_SECONDARY)
        return -1;
    if (!data || len > TWS_MEDIA_MAX)
        return -2;

    payload[0] = (u8)(tws->up_seq & 0xFF);
    payload[1] = (u8)(tws->up_seq >> 8);
    payload[2] = (u8)(timestamp & 0xFF);
    payload[3] = (u8)(timestamp >> 8);
    payload[4] = (u8)(timestamp >> 16);
    payload[5] = (u8)(timestamp >> 24);
    memcpy(&payload[6], data, len);

    int ret = tws_send(tws, TWS_MSG_MEDIA_UP, payload, (u16)(6 + len));
    if (!ret)
        tws->up_seq++;
    return ret;
}

int tws_send_key(struct tws *tws, u8 key, u8 state)
{
    u8 payload[2] = { key, state };
    return tws_send(tws, TWS_MSG_KEY, payload, sizeof(payload));
}

int tws_send_battery(struct tws *tws, u8 percent)
{
    tws->battery = percent;
    return tws_send(tws, TWS_MSG_BATTERY, &percent, 1);
}

int tws_request_role_switch(struct tws *tws)
{
    u8 target = (u8)(tws->role == TWS_ROLE_PRIMARY ?
                     TWS_ROLE_SECONDARY : TWS_ROLE_PRIMARY);

    tws->pending_role = target;
    struct lmp_link *link = lmp_link_find(tws->handle);
    if (link)
        lmp_role_switch(link);

    return tws_send(tws, TWS_MSG_ROLE_REQ, &target, 1);
}

static void tws_media_relay(const u8 *data, u16 len, u32 timestamp, void *arg)
{
    struct tws *tws = arg;
    if (tws)
        tws_relay_media(tws, data, len, timestamp);
}

void tws_attach_a2dp(struct tws *tws, struct a2dp *a2dp)
{
    if (tws && a2dp)
        a2dp_set_relay(a2dp, tws_media_relay, tws);
}

static void handle_hello(struct tws *tws, const u8 *data, u16 len)
{
    if (len < 8)
        return;
    memcpy(tws->phone.b, &data[2], 6);
    tws->sync_state = TWS_SYNC_READY;
    if (tws->cb.on_link)
        tws->cb.on_link(true, tws->arg);
}

static void handle_sync(struct tws *tws, const u8 *data, u16 len)
{
    if (len < 8)
        return;

    tws->clock_offset = (u32)data[0] | ((u32)data[1] << 8) |
                        ((u32)data[2] << 16) | ((u32)data[3] << 24);
    tws->anchor = (u32)data[4] | ((u32)data[5] << 8) |
                  ((u32)data[6] << 16) | ((u32)data[7] << 24);
    tws->stats.resyncs++;
}

static void handle_media_up(struct tws *tws, const u8 *data, u16 len)
{
    u32 timestamp;

    if (len < 6)
        return;
    timestamp = (u32)data[2] | ((u32)data[3] << 8) |
                ((u32)data[4] << 16) | ((u32)data[5] << 24);
    if (tws->cb.on_media_up)
        tws->cb.on_media_up(&data[6], (u16)(len - 6), timestamp, tws->arg);
}

static void handle_media(struct tws *tws, const u8 *data, u16 len)
{
    u32 timestamp;

    if (len < 6)
        return;

    timestamp = (u32)data[2] | ((u32)data[3] << 8) |
                ((u32)data[4] << 16) | ((u32)data[5] << 24);

    if (tws->cb.on_media)
        tws->cb.on_media(&data[6], (u16)(len - 6), timestamp, tws->arg);
}

void tws_rx(struct tws *tws, struct bt_buf *buf)
{
    u8 type;
    u16 seq, len;
    const u8 *hdr;
    const u8 *payload;

    if (!tws || !buf || bt_buf_len(buf) < TWS_HDR_LEN) {
        bt_buf_put(buf);
        return;
    }

    hdr = buf->data;
    hdr_read(hdr, &type, &seq, &len);
    if (bt_buf_len(buf) < TWS_HDR_LEN) {
        bt_buf_put(buf);
        return;
    }
    bt_buf_pull(buf, TWS_HDR_LEN);
    payload = buf->data;
    (void)seq;

    if (len > bt_buf_len(buf))
        len = bt_buf_len(buf);

    tws->stats.rx_msgs++;

    switch (type) {
    case TWS_MSG_HELLO:  handle_hello(tws, payload, len); break;
    case TWS_MSG_SYNC:   handle_sync(tws, payload, len); break;
    case TWS_MSG_MEDIA:  handle_media(tws, payload, len); break;
    case TWS_MSG_MEDIA_UP: handle_media_up(tws, payload, len); break;
    case TWS_MSG_KEY:
        if (len >= 2 && tws->cb.on_key)
            tws->cb.on_key(payload[0], payload[1], tws->arg);
        break;
    case TWS_MSG_BATTERY:
        if (len >= 1) {
            tws->battery = payload[0];
            if (tws->cb.on_battery)
                tws->cb.on_battery(payload[0], tws->arg);
        }
        break;
    case TWS_MSG_ROLE_REQ:
        if (len >= 1) {
            tws->role = (payload[0] == TWS_ROLE_PRIMARY) ?
                        TWS_ROLE_SECONDARY : TWS_ROLE_PRIMARY;
            tws_send(tws, TWS_MSG_ROLE_ACK, &tws->role, 1);
            if (tws->cb.on_role)
                tws->cb.on_role(tws->role, tws->arg);
        }
        break;
    case TWS_MSG_ROLE_ACK:
        tws->role = (enum tws_role)tws->pending_role;
        if (tws->cb.on_role)
            tws->cb.on_role(tws->role, tws->arg);
        break;
    case TWS_MSG_PING:
        tws_send(tws, TWS_MSG_PING, NULL, 0);
        break;
    default:
        tws->stats.dropped++;
        break;
    }

    bt_buf_put(buf);
}

void tws_close(struct tws *tws)
{
    if (!tws)
        return;
    if (tws->chan)
        l2cap_disconnect(tws->chan);
    tws->chan = NULL;
    tws->sync_state = TWS_SYNC_IDLE;
    if (tws->cb.on_link)
        tws->cb.on_link(false, tws->arg);
}
