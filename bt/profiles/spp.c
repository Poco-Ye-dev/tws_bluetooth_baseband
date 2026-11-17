#include <bt/spp.h>
#include <core/string.h>

static void spp_data(struct rfcomm_dlc *dlc, struct bt_buf *buf)
{
    struct spp *spp = dlc ? dlc->arg : NULL;
    if (spp && spp->rx)
        spp->rx(buf->data, bt_buf_len(buf), spp->arg);
    bt_buf_put(buf);
}

static const struct rfcomm_ops spp_ops = { .data = spp_data };

int spp_init(struct spp *spp, spp_rx_cb_t rx, void *arg)
{
    if (!spp)
        return -1;
    memset(spp, 0, sizeof(*spp));
    spp->rx = rx;
    spp->arg = arg;
    return rfcomm_mux_init(&spp->mux);
}

int spp_connect(struct spp *spp, const bt_addr *peer, u16 handle)
{
    (void)peer;

    if (!spp)
        return -1;
    spp->l2cap = l2cap_connect(RFCOMM_PSM, handle, 0, NULL, NULL);
    if (!spp->l2cap)
        return -2;
    rfcomm_mux_attach(&spp->mux, spp->l2cap);
    spp->dlc = rfcomm_connect(&spp->mux, 3, &spp_ops, spp);
    return spp->dlc ? 0 : -3;
}

int spp_send(struct spp *spp, const void *data, u16 len)
{
    return (spp && spp->dlc) ? rfcomm_send(spp->dlc, data, len) : -1;
}

void spp_rx(struct spp *spp, const u8 *data, u16 len)
{
    if (spp && spp->rx && data)
        spp->rx(data, len, spp->arg);
}

void spp_close(struct spp *spp)
{
    if (!spp)
        return;
    if (spp->dlc)
        rfcomm_disconnect(spp->dlc);
    if (spp->l2cap)
        l2cap_disconnect(spp->l2cap);
    spp->connected = false;
}
