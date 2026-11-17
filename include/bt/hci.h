#ifndef __BT_HCI_H__
#define __BT_HCI_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/buf.h>
#include <bt/transport.h>

/*
 * HCI core.
 *
 * Command flow is asynchronous.  A caller supplies a completion callback,
 * gets a command token and is notified from the event context.  Incoming ACL
 * fragments are reassembled per connection handle before being handed to
 * L2CAP.
 */

#define HCI_ACL_HANDLE_MASK 0x0FFF
#define HCI_ACL_PB_MASK     0x3000
#define HCI_ACL_BC_MASK     0xC000

enum hci_opcode {
    HCI_OP_RESET                  = 0x0C03,
    HCI_OP_READ_LOCAL_VERSION     = 0x1001,
    HCI_OP_READ_BD_ADDR           = 0x1009,
    HCI_OP_SET_EVENT_MASK         = 0x0C01,
    HCI_OP_WRITE_SCAN_ENABLE      = 0x0C1A,
    HCI_OP_DISCONNECT             = 0x0406,
    HCI_OP_CREATE_CONN            = 0x0405,
    HCI_OP_ACCEPT_CONN            = 0x0409,
    HCI_OP_LE_SET_ADV_PARAMS      = 0x2006,
    HCI_OP_LE_SET_ADV_DATA        = 0x2008,
    HCI_OP_LE_SET_SCAN_RSP_DATA   = 0x2009,
    HCI_OP_LE_SET_ADV_ENABLE      = 0x200A,
    HCI_OP_LE_SET_SCAN_PARAMS     = 0x200B,
    HCI_OP_LE_SET_SCAN_ENABLE     = 0x200C,
    HCI_OP_LE_CREATE_CONN         = 0x200D,
    HCI_OP_LE_CONN_UPDATE         = 0x2013,
};

enum hci_event {
    HCI_EV_DISCONN_COMPLETE   = 0x05,
    HCI_EV_ENCRYPT_CHANGE     = 0x08,
    HCI_EV_CMD_COMPLETE       = 0x0E,
    HCI_EV_CMD_STATUS         = 0x0F,
    HCI_EV_CONN_COMPLETE      = 0x03,
    HCI_EV_CONN_REQUEST       = 0x04,
    HCI_EV_LE_META            = 0x3E,
    HCI_EV_NUM_COMPLETED      = 0x13,
};

enum hci_le_subevent {
    HCI_LE_CONN_COMPLETE = 0x01,
    HCI_LE_ADV_REPORT    = 0x02,
    HCI_LE_CONN_UPDATE   = 0x03,
};

typedef void (*hci_cmd_cb_t)(u8 status, struct bt_buf *params, void *arg);
typedef void (*hci_event_cb_t)(const u8 *data, u16 len, void *arg);
typedef void (*hci_acl_cb_t)(u16 handle, u8 pb, struct bt_buf *buf);

struct hci_conn {
    u16 handle;
    bt_addr peer;
    enum bt_link_type type;
    bool encrypted;
    bool used;
};

int hci_init(void);
int hci_open(void);
int hci_close(void);
bool hci_ready(void);

int hci_send_cmd(u16 opcode, const void *params, u8 len,
                 hci_cmd_cb_t cb, void *arg);
int hci_send_acl(u16 handle, u8 pb, const void *data, u16 len);

void hci_register_event(u8 event, hci_event_cb_t cb, void *arg);
void hci_register_acl(hci_acl_cb_t cb);
void hci_register_le(u8 subevent, hci_event_cb_t cb, void *arg);

void hci_rx(const u8 *data, u16 len);
void hci_poll(void);

struct hci_conn *hci_conn_find(u16 handle);
struct hci_conn *hci_conn_find_addr(const bt_addr *addr);
struct hci_conn *hci_conn_alloc(u16 handle, const bt_addr *addr,
                                enum bt_link_type type);
void hci_conn_free(u16 handle);

/* Controller address, valid after hci_open() completes. */
const bt_addr *hci_local_addr(void);

#endif /* __BT_HCI_H__ */
