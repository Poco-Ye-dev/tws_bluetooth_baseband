#include <bt/obex.h>
#include <core/string.h>

static u16 put16(u8 *p, u16 v)
{
    p[0] = (u8)(v >> 8);
    p[1] = (u8)(v & 0xFF);
    return 2;
}

static void obex_data(struct rfcomm_dlc *dlc, struct bt_buf *buf)
{
    struct obex *obex = dlc ? dlc->arg : NULL;
    if (obex)
        obex_rx(obex, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

static const struct rfcomm_ops obex_ops = { .data = obex_data };

int obex_init(struct obex *obex,
              void (*on_data)(const u8 *, u16, void *), void *arg)
{
    if (!obex)
        return -1;
    memset(obex, 0, sizeof(*obex));
    obex->on_data = on_data;
    obex->arg = arg;
    return rfcomm_mux_init(&obex->mux);
}

int obex_connect(struct obex *obex, u16 handle, u8 dlci)
{
    if (!obex)
        return -1;
    obex->l2cap = l2cap_connect(RFCOMM_PSM, handle, 0, NULL, NULL);
    if (!obex->l2cap)
        return -2;
    rfcomm_mux_attach(&obex->mux, obex->l2cap);
    obex->dlc = rfcomm_connect(&obex->mux, dlci, &obex_ops, obex);
    if (!obex->dlc)
        return -3;

    u8 pkt[16];
    u16 pos = 0;
    pkt[pos++] = OBEX_CONNECT;
    pos += put16(&pkt[pos], 0x0004);
    pos += put16(&pkt[pos], 0x0010);
    pkt[pos++] = 0x10;               /* Target */
    pkt[pos++] = 0x00;
    pkt[pos++] = 0x00;
    pos += put16(&pkt[pos], 0x0001);
    return rfcomm_send(obex->dlc, pkt, pos);
}

int obex_put(struct obex *obex, const void *data, u16 len)
{
    u8 hdr[8];
    u16 pos = 0;

    if (!obex || !obex->dlc || !data)
        return -1;

    hdr[pos++] = OBEX_PUT_FINAL;
    pos += put16(&hdr[pos], (u16)(3 + len));
    rfcomm_send(obex->dlc, hdr, pos);
    return rfcomm_send(obex->dlc, data, len);
}

int obex_get(struct obex *obex, u8 type, const void *name, u16 name_len)
{
    u8 pkt[256];
    u16 pos = 0;

    if (!obex || !obex->dlc)
        return -1;

    pkt[pos++] = OBEX_GET_FINAL;
    pos += put16(&pkt[pos], (u16)(3 + 3 + name_len));
    pkt[pos++] = type;
    pos += put16(&pkt[pos], (u16)(name_len + 3));
    if (name && name_len) {
        memcpy(&pkt[pos], name, name_len);
        pos += name_len;
    }
    return rfcomm_send(obex->dlc, pkt, pos);
}

int obex_disconnect(struct obex *obex)
{
    u8 pkt[3] = { OBEX_DISCONNECT, 0x00, 0x03 };
    if (!obex || !obex->dlc)
        return -1;
    return rfcomm_send(obex->dlc, pkt, sizeof(pkt));
}

void obex_rx(struct obex *obex, const u8 *data, u16 len)
{
    if (!obex || !data || len < 3)
        return;

    u8 opcode = data[0];
    if (opcode == 0x90) {             /* CONNECT response */
        obex->connected = true;
    } else if (opcode == 0xA0) {      /* OK */
        if (obex->on_data)
            obex->on_data(&data[3], (u16)(len - 3), obex->arg);
    } else if (opcode == 0xC0 || opcode == 0xC3) {
        if (obex->on_data)
            obex->on_data(&data[3], (u16)(len - 3), obex->arg);
    }
}

void obex_close(struct obex *obex)
{
    if (!obex)
        return;
    if (obex->dlc)
        rfcomm_disconnect(obex->dlc);
    if (obex->l2cap)
        l2cap_disconnect(obex->l2cap);
    obex->connected = false;
}
