#include <bt/l2cap.h>
#include <bt/hci.h>
#include <bt/stack.h>
#include <core/string.h>
#include <core/printk.h>

#define L2CAP_MAX_CHANS 16
#define L2CAP_MAX_PSMS  8

#define SIG_CONN_REQ  0x02
#define SIG_CONN_RSP  0x03
#define SIG_CONFIG_REQ 0x04
#define SIG_CONFIG_RSP 0x05
#define SIG_DISCONN_REQ 0x06
#define SIG_DISCONN_RSP 0x07

#define CONFIG_SENT 0x01
#define CONFIG_RCVD 0x02
#define CONFIG_DONE 0x03

struct psm_entry {
    u16 psm;
    const struct l2cap_ops *ops;
    void *arg;
};

static struct l2cap_chan chans[L2CAP_MAX_CHANS];
static struct psm_entry psms[L2CAP_MAX_PSMS];
static u8 psm_count;
static u16 next_cid = L2CAP_CID_DYNAMIC;
static u8 signal_id;

struct fixed_entry {
    struct l2cap_chan *chan;
    const struct l2cap_ops *ops;
    void *arg;
};
static struct fixed_entry fixed[8];

static struct l2cap_chan *chan_alloc(void)
{
    for (unsigned int i = 0; i < L2CAP_MAX_CHANS; i++) {
        if (chans[i].state == L2CAP_CLOSED && !chans[i].dynamic_cid) {
            memset(&chans[i], 0, sizeof(chans[i]));
            chans[i].state = L2CAP_CLOSED;
            return &chans[i];
        }
    }
    return NULL;
}

static struct l2cap_chan *chan_by_scid(u16 scid)
{
    for (unsigned int i = 0; i < L2CAP_MAX_CHANS; i++) {
        if (chans[i].state != L2CAP_CLOSED && chans[i].scid == scid)
            return &chans[i];
    }
    return NULL;
}

static struct l2cap_chan *chan_by_dcid(u16 dcid)
{
    for (unsigned int i = 0; i < L2CAP_MAX_CHANS; i++) {
        if (chans[i].state != L2CAP_CLOSED && chans[i].dcid == dcid)
            return &chans[i];
    }
    return NULL;
}

struct l2cap_chan *l2cap_chan_by_cid(u16 cid)
{
    struct l2cap_chan *chan = chan_by_scid(cid);
    return chan ? chan : chan_by_dcid(cid);
}

static u16 alloc_cid(void)
{
    u16 cid = next_cid++;
    if (next_cid < L2CAP_CID_DYNAMIC)
        next_cid = L2CAP_CID_DYNAMIC;
    return cid;
}

static int send_signal(u16 handle, u8 code, u8 id,
                       const void *payload, u16 len)
{
    struct bt_buf *buf = bt_buf_alloc(4 + 4);
    if (!buf)
        return -1;

    u16 total = (u16)(4 + len);
    u8 hdr[4] = {
        (u8)(total & 0xFF), (u8)(total >> 8),
        (u8)(L2CAP_CID_SIGNALING & 0xFF), (u8)(L2CAP_CID_SIGNALING >> 8),
    };
    bt_buf_append(buf, hdr, 4);
    u8 sig[4] = { code, id, (u8)(len & 0xFF), (u8)(len >> 8) };
    bt_buf_append(buf, sig, 4);
    if (len && payload)
        bt_buf_append(buf, payload, len);

    int ret = hci_send_acl(handle, 0x2, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    return ret;
}

static void send_config_req(struct l2cap_chan *chan)
{
    u8 payload[6];
    payload[0] = (u8)(chan->dcid & 0xFF);
    payload[1] = (u8)(chan->dcid >> 8);
    payload[2] = 0x00;
    payload[3] = 0x01;
    payload[4] = 0x02;              /* MTU option */
    payload[5] = 0x02;

    send_signal(chan->handle, SIG_CONFIG_REQ, signal_id++, payload, 6);
    chan->config_state |= CONFIG_SENT;
}

static void chan_maybe_open(struct l2cap_chan *chan)
{
    if (chan->state == L2CAP_WAIT_CONFIG &&
        (chan->config_state & CONFIG_DONE) == CONFIG_DONE) {
        chan->state = L2CAP_OPEN;
        if (chan->ops && chan->ops->connected)
            chan->ops->connected(chan);
    }
}

static void handle_connect_req(u16 handle, u8 id, const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 psm = (u16)(data[0] | (data[1] << 8));
    u16 rscid = (u16)(data[2] | (data[3] << 8));
    const struct l2cap_ops *ops = NULL;
    void *arg = NULL;

    for (unsigned int i = 0; i < psm_count; i++) {
        if (psms[i].psm == psm) {
            ops = psms[i].ops;
            arg = psms[i].arg;
            break;
        }
    }

    struct l2cap_chan *chan = chan_alloc();
    u8 rsp[8];

    if (!chan || !ops) {
        rsp[0] = (u8)(rscid & 0xFF);
        rsp[1] = (u8)(rscid >> 8);
        rsp[2] = 0;
        rsp[3] = 0;
        rsp[4] = 0x02;              /* PSM not supported */
        rsp[5] = 0;
        send_signal(handle, SIG_CONN_RSP, id, rsp, 6);
        return;
    }

    chan->handle = handle;
    chan->psm = psm;
    chan->scid = alloc_cid();
    chan->dcid = rscid;
    chan->dynamic_cid = true;
    chan->mtu = L2CAP_DEFAULT_MTU;
    chan->initiator = false;
    chan->ops = ops;
    chan->arg = arg;
    chan->state = L2CAP_WAIT_CONFIG;

    rsp[0] = (u8)(chan->dcid & 0xFF);
    rsp[1] = (u8)(chan->dcid >> 8);
    rsp[2] = (u8)(chan->scid & 0xFF);
    rsp[3] = (u8)(chan->scid >> 8);
    rsp[4] = 0x00;
    rsp[5] = 0x00;
    send_signal(handle, SIG_CONN_RSP, id, rsp, 6);
    send_config_req(chan);
}

static void handle_connect_rsp(u8 id, const u8 *data, u16 len)
{
    if (len < 6)
        return;

    u16 scid = (u16)(data[2] | (data[3] << 8));
    u8 result = data[4];
    struct l2cap_chan *chan = chan_by_scid(scid);

    if (!chan)
        return;

    if (result == 0) {
        chan->dcid = (u16)(data[0] | (data[1] << 8));
        chan->state = L2CAP_WAIT_CONFIG;
        send_config_req(chan);
    } else {
        chan->state = L2CAP_CLOSED;
        if (chan->ops && chan->ops->disconnected)
            chan->ops->disconnected(chan, data[5]);
    }
    (void)id;
}

static void handle_config_req(u16 handle, u8 id, const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 dcid = (u16)(data[0] | (data[1] << 8));
    u8 rsp[6];
    struct l2cap_chan *chan = chan_by_scid(dcid);

    rsp[0] = data[0];
    rsp[1] = data[1];
    rsp[2] = 0;
    rsp[3] = 0;
    rsp[4] = chan ? 0x00 : 0x02;
    rsp[5] = 0;

    if (chan) {
        chan->config_state |= CONFIG_RCVD;
        send_config_req(chan);
    }

    send_signal(handle, SIG_CONFIG_RSP, id, rsp, 6);
}

static void handle_config_rsp(u8 id, const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 scid = (u16)(data[0] | (data[1] << 8));
    struct l2cap_chan *chan = chan_by_scid(scid);

    if (!chan)
        return;

    chan->config_state |= CONFIG_SENT;
    if (chan->config_state & CONFIG_RCVD)
        chan->config_state |= CONFIG_DONE;

    chan_maybe_open(chan);
    (void)id;
}

static void handle_disconnect_req(u16 handle, u8 id, const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 dcid = (u16)(data[0] | (data[1] << 8));
    u16 scid = (u16)(data[2] | (data[3] << 8));
    struct l2cap_chan *chan = chan_by_scid(dcid);

    send_signal(handle, SIG_DISCONN_RSP, id, data, 4);

    if (chan) {
        if (chan->ops && chan->ops->disconnected)
            chan->ops->disconnected(chan, 0);
        chan->state = L2CAP_CLOSED;
        chan->dynamic_cid = false;
    }
    (void)scid;
}

static void handle_disconnect_rsp(const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 scid = (u16)(data[0] | (data[1] << 8));
    struct l2cap_chan *chan = chan_by_scid(scid);

    if (!chan)
        return;

    if (chan->ops && chan->ops->disconnected)
        chan->ops->disconnected(chan, 0);
    chan->state = L2CAP_CLOSED;
    chan->dynamic_cid = false;
}

static void handle_signaling(u16 handle, struct bt_buf *buf)
{
    while (bt_buf_len(buf) >= 4) {
        u8 code = bt_buf_get_u8(buf);
        u8 id = bt_buf_get_u8(buf);
        u16 len = bt_buf_get_le16(buf);
        const u8 *data = bt_buf_pull(buf, len);

        if (!data && len)
            break;

        switch (code) {
        case SIG_CONN_REQ:
            handle_connect_req(handle, id, data, len);
            break;
        case SIG_CONN_RSP:
            handle_connect_rsp(id, data, len);
            break;
        case SIG_CONFIG_REQ:
            handle_config_req(handle, id, data, len);
            break;
        case SIG_CONFIG_RSP:
            handle_config_rsp(id, data, len);
            break;
        case SIG_DISCONN_REQ:
            handle_disconnect_req(handle, id, data, len);
            break;
        case SIG_DISCONN_RSP:
            handle_disconnect_rsp(data, len);
            break;
        default:
            break;
        }
    }
}

static void l2cap_acl_rx(u16 handle, u8 pb, struct bt_buf *buf)
{
    (void)pb;

    u16 len = bt_buf_get_le16(buf);
    u16 cid = bt_buf_get_le16(buf);

    if (len > bt_buf_len(buf))
        len = bt_buf_len(buf);

    if (cid == L2CAP_CID_SIGNALING) {
        handle_signaling(handle, buf);
        bt_buf_put(buf);
        return;
    }

    if (cid < 8 && fixed[cid].chan) {
        fixed[cid].chan->handle = handle;
        fixed[cid].chan->dcid = cid;
        fixed[cid].chan->scid = cid;
        if (fixed[cid].ops && fixed[cid].ops->data)
            fixed[cid].ops->data(fixed[cid].chan, buf);
        else
            bt_buf_put(buf);
        return;
    }

    struct l2cap_chan *chan = l2cap_chan_by_cid(cid);
    if (!chan) {
        bt_buf_put(buf);
        return;
    }

    if (chan->ops && chan->ops->data)
        chan->ops->data(chan, buf);
    else
        bt_buf_put(buf);
}

int l2cap_register_fixed(u16 cid, const struct l2cap_ops *ops, void *arg)
{
    if (cid >= 8 || fixed[cid].chan)
        return -1;

    struct l2cap_chan *chan = chan_alloc();
    if (!chan)
        return -2;

    chan->scid = cid;
    chan->dcid = cid;
    chan->state = L2CAP_OPEN;
    chan->dynamic_cid = true;
    chan->ops = ops;
    chan->arg = arg;
    fixed[cid].chan = chan;
    fixed[cid].ops = ops;
    fixed[cid].arg = arg;
    return 0;
}

struct l2cap_chan *l2cap_fixed_chan(u16 cid)
{
    return cid < 8 ? fixed[cid].chan : NULL;
}

int l2cap_init(void)
{
    memset(chans, 0, sizeof(chans));
    memset(psms, 0, sizeof(psms));
    psm_count = 0;
    next_cid = L2CAP_CID_DYNAMIC;
    signal_id = 1;
    hci_register_acl(l2cap_acl_rx);
    return 0;
}

int l2cap_register_psm(u16 psm, const struct l2cap_ops *ops, void *arg)
{
    if (!ops || psm_count >= L2CAP_MAX_PSMS)
        return -1;

    psms[psm_count].psm = psm;
    psms[psm_count].ops = ops;
    psms[psm_count].arg = arg;
    psm_count++;
    return 0;
}

struct l2cap_chan *l2cap_connect(u16 psm, u16 handle, u16 mtu,
                                 const struct l2cap_ops *ops, void *arg)
{
    struct l2cap_chan *chan = chan_alloc();
    if (!chan)
        return NULL;

    chan->handle = handle;
    chan->psm = psm;
    chan->scid = alloc_cid();
    chan->dcid = 0;
    chan->mtu = mtu ? mtu : L2CAP_DEFAULT_MTU;
    chan->dynamic_cid = true;
    chan->initiator = true;
    chan->ops = ops;
    chan->arg = arg;
    chan->state = L2CAP_WAIT_CONNECT;

    u8 payload[4] = {
        (u8)(psm & 0xFF), (u8)(psm >> 8),
        (u8)(chan->scid & 0xFF), (u8)(chan->scid >> 8),
    };
    send_signal(handle, SIG_CONN_REQ, signal_id++, payload, 4);
    return chan;
}

int l2cap_send(struct l2cap_chan *chan, const void *data, u16 len)
{
    struct bt_buf *buf;

    if (!chan || chan->state != L2CAP_OPEN)
        return -1;
    if (len > chan->mtu)
        return -2;

    buf = bt_buf_alloc(4);
    if (!buf)
        return -3;

    u8 hdr[4] = {
        (u8)(len & 0xFF), (u8)(len >> 8),
        (u8)(chan->dcid & 0xFF), (u8)(chan->dcid >> 8),
    };
    bt_buf_append(buf, hdr, 4);
    bt_buf_append(buf, data, len);

    int ret = hci_send_acl(chan->handle, 0x2, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    return ret;
}

int l2cap_disconnect(struct l2cap_chan *chan)
{
    u8 payload[4];

    if (!chan)
        return -1;

    payload[0] = (u8)(chan->dcid & 0xFF);
    payload[1] = (u8)(chan->dcid >> 8);
    payload[2] = (u8)(chan->scid & 0xFF);
    payload[3] = (u8)(chan->scid >> 8);

    chan->state = L2CAP_WAIT_DISCONNECT;
    return send_signal(chan->handle, SIG_DISCONN_REQ, signal_id++, payload, 4);
}

void l2cap_rx(u16 handle, struct bt_buf *buf)
{
    l2cap_acl_rx(handle, 0, buf);
}
