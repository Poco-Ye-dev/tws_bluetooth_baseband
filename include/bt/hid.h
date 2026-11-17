#ifndef __BT_HID_H__
#define __BT_HID_H__

#include <core/types.h>
#include <bt/l2cap.h>

/*
 * HID host device: control and interrupt channels, report forwarding.
 */

#define HID_PSM_CONTROL   0x0011
#define HID_PSM_INTERRUPT 0x0013

struct hid {
    struct l2cap_chan *control;
    struct l2cap_chan *interrupt;
    u8 protocol;
    void (*on_report)(u8 type, const u8 *data, u16 len, void *arg);
    void *arg;
};

int hid_init(struct hid *hid, void (*on_report)(u8, const u8 *, u16, void *),
             void *arg);
int hid_connect(struct hid *hid, u16 handle);
int hid_send_report(struct hid *hid, u8 type, const u8 *data, u16 len);
int hid_set_protocol(struct hid *hid, u8 protocol);
void hid_rx(struct hid *hid, struct bt_buf *buf);
void hid_close(struct hid *hid);

#endif /* __BT_HID_H__ */
