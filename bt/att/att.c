#include <bt/att.h>
#include <core/errno.h>
#include <core/string.h>
#include <core/printk.h>

static void att_send(struct l2cap_chan *chan, struct bt_buf *buf)
{
    if (!chan || !buf) {
        bt_buf_put(buf);
        return;
    }
    l2cap_send(chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

static void att_error(struct att_db *db, u8 opcode, u16 handle, u8 error)
{
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return;

    u8 pdu[5] = { ATT_OP_ERROR_RSP, opcode,
                  (u8)(handle & 0xFF), (u8)(handle >> 8), error };
    bt_buf_append(buf, pdu, sizeof(pdu));
    att_send(db->chan, buf);
}

int att_server_init(struct att_db *db)
{
    if (!db)
        return -EINVAL;

    memset(db, 0, sizeof(*db));
    db->next_handle = 1;
    db->mtu = ATT_MTU_DEFAULT;
    return 0;
}

u16 att_add_attr(struct att_db *db, const bt_uuid *type, u8 props,
                 att_read_cb_t read, att_write_cb_t write, void *arg)
{
    struct att_attr *attr;

    if (!db || !type || db->count >= ATT_MAX_ATTRS)
        return ATT_INVALID_HANDLE;

    attr = &db->attr[db->count++];
    attr->handle = db->next_handle++;
    attr->type = *type;
    attr->props = props;
    attr->read = read;
    attr->write = write;
    attr->arg = arg;
    return attr->handle;
}

struct att_attr *att_find(struct att_db *db, u16 handle)
{
    for (unsigned int i = 0; i < db->count; i++) {
        if (db->attr[i].handle == handle)
            return &db->attr[i];
    }
    return NULL;
}

void att_server_attach(struct att_db *db, struct l2cap_chan *chan)
{
    if (db)
        db->chan = chan;
}

static void att_read_req(struct att_db *db, struct bt_buf *buf)
{
    u16 handle = bt_buf_get_le16(buf);
    struct att_attr *attr = att_find(db, handle);

    if (!attr) {
        att_error(db, ATT_OP_READ_REQ, handle, ATT_ERR_INVALID_HANDLE);
        return;
    }
    if (!(attr->props & ATT_PROP_READ) || !attr->read) {
        att_error(db, ATT_OP_READ_REQ, handle, ATT_ERR_READ_NOT_PERMITTED);
        return;
    }

    struct bt_buf *rsp = bt_buf_alloc(0);
    if (!rsp)
        return;
    bt_buf_put_data(rsp, 1);

    u8 *value = rsp->data + 1;
    int len = attr->read(handle, value, (u16)(db->mtu - 1), attr->arg);
    if (len < 0) {
        bt_buf_put(rsp);
        att_error(db, ATT_OP_READ_REQ, handle, ATT_ERR_NOT_FOUND);
        return;
    }

    rsp->data[0] = ATT_OP_READ_RSP;
    rsp->tail = value + len;
    att_send(db->chan, rsp);
}

static void att_write(struct att_db *db, struct bt_buf *buf, bool need_rsp)
{
    u16 handle = bt_buf_get_le16(buf);
    u16 len = (u16)bt_buf_len(buf);
    const void *value = bt_buf_pull(buf, len);
    struct att_attr *attr = att_find(db, handle);

    if (!attr) {
        if (need_rsp)
            att_error(db, need_rsp ? ATT_OP_WRITE_REQ : ATT_OP_WRITE_CMD,
                      handle, ATT_ERR_INVALID_HANDLE);
        return;
    }
    if (!(attr->props & ATT_PROP_WRITE) || !attr->write) {
        if (need_rsp)
            att_error(db, ATT_OP_WRITE_REQ, handle, ATT_ERR_WRITE_NOT_PERMITTED);
        return;
    }

    int ret = attr->write(handle, value, len, attr->arg);
    if (!need_rsp)
        return;

    if (ret) {
        att_error(db, ATT_OP_WRITE_REQ, handle, ATT_ERR_INVALID_VALUE);
        return;
    }

    struct bt_buf *rsp = bt_buf_alloc(0);
    if (!rsp)
        return;
    bt_buf_append(rsp, "\x13", 1);
    att_send(db->chan, rsp);
}

static void att_read_by_group(struct att_db *db, struct bt_buf *buf)
{
    u16 start = bt_buf_get_le16(buf);
    u16 end = bt_buf_get_le16(buf);
    bt_uuid type = { .len = 2, .b = { 0, 0 } };
    bt_buf_read(buf, type.b, 2);

    struct bt_buf *rsp = bt_buf_alloc(0);
    if (!rsp)
        return;

    u8 hdr[2] = { ATT_OP_READ_BY_GROUP_RSP, 0 };
    bt_buf_append(rsp, hdr, 2);

    u8 entry_len = 0;
    for (unsigned int i = 0; i < db->count; i++) {
        struct att_attr *attr = &db->attr[i];
        if (attr->handle < start || attr->handle > end)
            continue;
        if (!bt_uuid_eq(&attr->type, &type) || !attr->read)
            continue;

        u8 value[16];
        int vlen = attr->read(attr->handle, value, sizeof(value), attr->arg);
        if (vlen <= 0)
            continue;

        u8 entry[4 + 16];
        entry[0] = (u8)(attr->handle & 0xFF);
        entry[1] = (u8)(attr->handle >> 8);
        entry[2] = 0xFF;
        entry[3] = 0xFF;
        memcpy(&entry[4], value, (size_t)vlen);

        if (bt_buf_len(rsp) + 4 + vlen > db->mtu - 1)
            break;

        bt_buf_append(rsp, entry, (u16)(4 + vlen));
        entry_len = (u8)(4 + vlen);
    }

    if (!entry_len) {
        bt_buf_put(rsp);
        att_error(db, ATT_OP_READ_BY_GROUP_REQ, start, ATT_ERR_NOT_FOUND);
        return;
    }

    rsp->data[1] = entry_len;
    att_send(db->chan, rsp);
}

static void att_read_by_type(struct att_db *db, struct bt_buf *buf)
{
    u16 start = bt_buf_get_le16(buf);
    u16 end = bt_buf_get_le16(buf);
    bt_uuid type = { .len = 2, .b = { 0, 0 } };
    bt_buf_read(buf, type.b, 2);

    struct bt_buf *rsp = bt_buf_alloc(0);
    if (!rsp)
        return;

    u8 hdr[2] = { ATT_OP_READ_BY_TYPE_RSP, 0 };
    bt_buf_append(rsp, hdr, 2);

    u8 entry_len = 0;
    for (unsigned int i = 0; i < db->count; i++) {
        struct att_attr *attr = &db->attr[i];
        if (attr->handle < start || attr->handle > end)
            continue;
        if (!bt_uuid_eq(&attr->type, &type) || !attr->read)
            continue;

        u8 value[32];
        int vlen = attr->read(attr->handle, value, sizeof(value), attr->arg);
        if (vlen < 0)
            continue;

        u8 entry[2 + 32];
        entry[0] = (u8)(attr->handle & 0xFF);
        entry[1] = (u8)(attr->handle >> 8);
        memcpy(&entry[2], value, (size_t)vlen);

        if (bt_buf_len(rsp) + 2 + vlen > db->mtu - 1)
            break;

        bt_buf_append(rsp, entry, (u16)(2 + vlen));
        entry_len = (u8)(2 + vlen);
    }

    if (!entry_len) {
        bt_buf_put(rsp);
        att_error(db, ATT_OP_READ_BY_TYPE_REQ, start, ATT_ERR_NOT_FOUND);
        return;
    }

    rsp->data[1] = entry_len;
    att_send(db->chan, rsp);
}

static void att_find_info(struct att_db *db, struct bt_buf *buf)
{
    u16 start = bt_buf_get_le16(buf);
    u16 end = bt_buf_get_le16(buf);

    struct bt_buf *rsp = bt_buf_alloc(0);
    if (!rsp)
        return;

    u8 hdr[2] = { ATT_OP_FIND_INFO_RSP, 0x01 };
    bt_buf_append(rsp, hdr, 2);

    for (unsigned int i = 0; i < db->count; i++) {
        struct att_attr *attr = &db->attr[i];
        if (attr->handle < start || attr->handle > end)
            continue;
        if (bt_buf_len(rsp) + 4 > db->mtu - 1)
            break;

        u8 entry[4] = {
            (u8)(attr->handle & 0xFF), (u8)(attr->handle >> 8),
            attr->type.b[0], attr->type.b[1],
        };
        bt_buf_append(rsp, entry, sizeof(entry));
    }

    if (bt_buf_len(rsp) == 2) {
        bt_buf_put(rsp);
        att_error(db, ATT_OP_FIND_INFO_REQ, start, ATT_ERR_NOT_FOUND);
        return;
    }
    att_send(db->chan, rsp);
}

void att_server_rx(struct att_db *db, struct bt_buf *buf)
{
    if (!db || !buf)
        return;

    u8 opcode = bt_buf_get_u8(buf);

    switch (opcode) {
    case ATT_OP_MTU_REQ: {
        u16 mtu = bt_buf_get_le16(buf);
        if (mtu > 517)
            mtu = 517;
        db->mtu = mtu;
        struct bt_buf *rsp = bt_buf_alloc(0);
        u8 pdu[3] = { ATT_OP_MTU_RSP, (u8)(mtu & 0xFF), (u8)(mtu >> 8) };
        bt_buf_append(rsp, pdu, sizeof(pdu));
        att_send(db->chan, rsp);
        break;
    }
    case ATT_OP_READ_REQ:
        att_read_req(db, buf);
        break;
    case ATT_OP_WRITE_REQ:
        att_write(db, buf, true);
        break;
    case ATT_OP_WRITE_CMD:
        att_write(db, buf, false);
        break;
    case ATT_OP_READ_BY_GROUP_REQ:
        att_read_by_group(db, buf);
        break;
    case ATT_OP_READ_BY_TYPE_REQ:
        att_read_by_type(db, buf);
        break;
    case ATT_OP_FIND_INFO_REQ:
        att_find_info(db, buf);
        break;
    case ATT_OP_CONFIRM:
        break;
    default:
        att_error(db, opcode, 0, ATT_ERR_REQUEST_NOT_SUPPORTED);
        break;
    }

    bt_buf_put(buf);
}

static int att_simple_value(struct bt_buf *buf, u16 handle, const void *data,
                            u16 len)
{
    if (!buf)
        return -1;

    u8 pdu[3 + 128];
    if (len > sizeof(pdu) - 3)
        len = sizeof(pdu) - 3;

    pdu[0] = ATT_OP_NOTIFY;
    pdu[1] = (u8)(handle & 0xFF);
    pdu[2] = (u8)(handle >> 8);
    memcpy(&pdu[3], data, len);
    bt_buf_append(buf, pdu, (u16)(3 + len));
    return 0;
}

int att_notify(struct att_db *db, u16 handle, const void *data, u16 len)
{
    struct bt_buf *buf;

    if (!db || !db->chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    att_simple_value(buf, handle, data, len);
    att_send(db->chan, buf);
    return 0;
}

int att_indicate(struct att_db *db, u16 handle, const void *data, u16 len)
{
    struct bt_buf *buf;

    if (!db || !db->chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    att_simple_value(buf, handle, data, len);
    buf->data[0] = ATT_OP_INDICATE;
    att_send(db->chan, buf);
    return 0;
}

static int att_client_pdu(struct l2cap_chan *chan, u8 opcode,
                          const void *payload, u16 len)
{
    struct bt_buf *buf;

    if (!chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    bt_buf_append(buf, &opcode, 1);
    bt_buf_append(buf, payload, len);
    int ret = l2cap_send(chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    return ret;
}

int att_client_read(struct l2cap_chan *chan, u16 handle)
{
    u8 pdu[2] = { (u8)(handle & 0xFF), (u8)(handle >> 8) };
    return att_client_pdu(chan, ATT_OP_READ_REQ, pdu, sizeof(pdu));
}

int att_client_write(struct l2cap_chan *chan, u16 handle,
                     const void *data, u16 len)
{
    u8 pdu[2 + 128];
    if (len > sizeof(pdu) - 2)
        return -1;
    pdu[0] = (u8)(handle & 0xFF);
    pdu[1] = (u8)(handle >> 8);
    memcpy(&pdu[2], data, len);
    return att_client_pdu(chan, ATT_OP_WRITE_REQ, pdu, (u16)(2 + len));
}

int att_client_find_info(struct l2cap_chan *chan, u16 start, u16 end)
{
    u8 pdu[4] = { (u8)(start & 0xFF), (u8)(start >> 8),
                  (u8)(end & 0xFF), (u8)(end >> 8) };
    return att_client_pdu(chan, ATT_OP_FIND_INFO_REQ, pdu, sizeof(pdu));
}

int att_client_read_by_group(struct l2cap_chan *chan, u16 start, u16 end,
                             const bt_uuid *type)
{
    u8 pdu[2 + 2 + 16];
    u16 len = 4;

    pdu[0] = (u8)(start & 0xFF);
    pdu[1] = (u8)(start >> 8);
    pdu[2] = (u8)(end & 0xFF);
    pdu[3] = (u8)(end >> 8);
    if (type) {
        memcpy(&pdu[4], type->b, type->len);
        len = (u16)(4 + type->len);
    }
    return att_client_pdu(chan, ATT_OP_READ_BY_GROUP_REQ, pdu, len);
}
