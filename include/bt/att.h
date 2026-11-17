#ifndef __BT_ATT_H__
#define __BT_ATT_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/buf.h>
#include <bt/l2cap.h>

/*
 * ATT attribute server and client.
 *
 * The server is a flat, handle ordered attribute table.  Each entry owns a
 * type, permissions and an optional access callback, which is enough to model
 * GATT services without hard-coding any profile.
 */

#define ATT_MTU_DEFAULT 23
#define ATT_MAX_ATTRS   64
#define ATT_MAX_HANDLE  0xFFFF
#define ATT_INVALID_HANDLE 0x0000

enum att_opcode {
    ATT_OP_ERROR_RSP        = 0x01,
    ATT_OP_MTU_REQ          = 0x02,
    ATT_OP_MTU_RSP          = 0x03,
    ATT_OP_FIND_INFO_REQ    = 0x04,
    ATT_OP_FIND_INFO_RSP    = 0x05,
    ATT_OP_READ_BY_TYPE_REQ = 0x08,
    ATT_OP_READ_BY_TYPE_RSP = 0x09,
    ATT_OP_READ_REQ         = 0x0A,
    ATT_OP_READ_RSP         = 0x0B,
    ATT_OP_READ_BY_GROUP_REQ = 0x10,
    ATT_OP_READ_BY_GROUP_RSP = 0x11,
    ATT_OP_WRITE_REQ        = 0x12,
    ATT_OP_WRITE_RSP        = 0x13,
    ATT_OP_WRITE_CMD        = 0x52,
    ATT_OP_NOTIFY           = 0x1B,
    ATT_OP_INDICATE         = 0x1D,
    ATT_OP_CONFIRM          = 0x1E,
    ATT_OP_HANDLE_VALUE_NTF = 0x1B,
    ATT_OP_HANDLE_VALUE_IND = 0x1D,
};

enum att_error {
    ATT_ERR_INVALID_HANDLE = 0x01,
    ATT_ERR_READ_NOT_PERMITTED = 0x02,
    ATT_ERR_WRITE_NOT_PERMITTED = 0x03,
    ATT_ERR_INVALID_PDU = 0x04,
    ATT_ERR_INSUFFICIENT_AUTH = 0x05,
    ATT_ERR_REQUEST_NOT_SUPPORTED = 0x06,
    ATT_ERR_INVALID_OFFSET = 0x07,
    ATT_ERR_INSUFFICIENT_KEY = 0x08,
    ATT_ERR_INVALID_VALUE = 0x0D,
    ATT_ERR_NOT_FOUND = 0x0A,
    ATT_ERR_ATTRIBUTE_NOT_FOUND = 0x0A,
};

#define ATT_PROP_READ    0x02
#define ATT_PROP_WRITE   0x08
#define ATT_PROP_NOTIFY  0x10
#define ATT_PROP_INDICATE 0x20

typedef int (*att_read_cb_t)(u16 handle, void *buf, u16 max, void *arg);
typedef int (*att_write_cb_t)(u16 handle, const void *buf, u16 len, void *arg);

struct att_attr {
    u16 handle;
    bt_uuid type;
    u8 props;
    att_read_cb_t read;
    att_write_cb_t write;
    void *arg;
};

struct att_db {
    struct att_attr attr[ATT_MAX_ATTRS];
    u16 count;
    u16 next_handle;
    u16 mtu;
    struct l2cap_chan *chan;
};

int att_server_init(struct att_db *db);
u16 att_add_attr(struct att_db *db, const bt_uuid *type, u8 props,
                 att_read_cb_t read, att_write_cb_t write, void *arg);
struct att_attr *att_find(struct att_db *db, u16 handle);
void att_server_attach(struct att_db *db, struct l2cap_chan *chan);
void att_server_rx(struct att_db *db, struct bt_buf *buf);

int att_notify(struct att_db *db, u16 handle, const void *data, u16 len);
int att_indicate(struct att_db *db, u16 handle, const void *data, u16 len);

/* Client helpers used by GATT discovery. */
int att_client_read(struct l2cap_chan *chan, u16 handle);
int att_client_write(struct l2cap_chan *chan, u16 handle,
                     const void *data, u16 len);
int att_client_find_info(struct l2cap_chan *chan, u16 start, u16 end);
int att_client_read_by_group(struct l2cap_chan *chan, u16 start, u16 end,
                             const bt_uuid *type);

#endif /* __BT_ATT_H__ */
