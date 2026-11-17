#include <bt/hid.h>
#include <core/string.h>

int hid_init(struct hid *hid, void (*on_report)(u8, const u8 *, u16, void *),
             void *arg)
{
    if (!hid)
        return -1;
    memset(hid, 0, sizeof(*hid));
    hid->on_report = on_report;
    hid->arg = arg;
    hid->protocol = 1;               /* report protocol */
    return 0;
}

int hid_connect(struct hid *hid, u16 handle)
{
    if (!hid)
        return -1;
    hid->control = l2cap_connect(HID_PSM_CONTROL, handle, 0, NULL, NULL);
    hid->interrupt = l2cap_connect(HID_PSM_INTERRUPT, handle, 0, NULL, NULL);
    return (hid->control && hid->interrupt) ? 0 : -2;
}

int hid_set_protocol(struct hid *hid, u8 protocol)
{
    u8 pdu[2] = { 0x70, protocol };

    if (!hid || !hid->control)
        return -1;
    hid->protocol = protocol;
    return l2cap_send(hid->control, pdu, sizeof(pdu));
}

int hid_send_report(struct hid *hid, u8 type, const u8 *data, u16 len)
{
    u8 pdu[2 + 64];

    if (!hid || !hid->interrupt || !data || len > 64)
        return -1;

    pdu[0] = (u8)(0xA0 | (type & 0x0F));
    pdu[1] = (u8)len;
    memcpy(&pdu[2], data, len);
    return l2cap_send(hid->interrupt, pdu, (u16)(2 + len));
}

void hid_rx(struct hid *hid, struct bt_buf *buf)
{
    if (!hid || !buf || bt_buf_len(buf) < 1) {
        bt_buf_put(buf);
        return;
    }

    u8 type = bt_buf_get_u8(buf);
    u16 len = bt_buf_len(buf);

    if (hid->on_report)
        hid->on_report((u8)(type & 0x0F), buf->data, len, hid->arg);
    bt_buf_put(buf);
}

void hid_close(struct hid *hid)
{
    if (!hid)
        return;
    if (hid->control)
        l2cap_disconnect(hid->control);
    if (hid->interrupt)
        l2cap_disconnect(hid->interrupt);
}
