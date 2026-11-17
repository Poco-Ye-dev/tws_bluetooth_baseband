#include <bt/sdp.h>
#include <core/string.h>
#include <core/printk.h>

#define SDP_PDU_ERROR_RSP        0x01
#define SDP_PDU_SEARCH_REQ       0x02
#define SDP_PDU_SEARCH_RSP       0x03
#define SDP_PDU_ATTR_REQ         0x04
#define SDP_PDU_ATTR_RSP         0x05

static u32 next_handle = 0x10000;

u32 sdp_next_handle(void)
{
    return next_handle++;
}

int sdp_server_init(struct sdp_server *server)
{
    if (!server)
        return -1;
    memset(server, 0, sizeof(*server));
    return 0;
}

int sdp_register_record(struct sdp_server *server, struct sdp_record *record)
{
    if (!server || !record || server->count >= SDP_MAX_RECORDS)
        return -1;

    if (!record->handle)
        record->handle = sdp_next_handle();
    server->record[server->count++] = record;
    return 0;
}

static bool record_matches(const struct sdp_record *record,
                           const bt_uuid *uuid)
{
    for (unsigned int i = 0; i < record->count; i++) {
        if (record->attr[i].id != SDP_ATTR_SERVICE_CLASS_IDS)
            continue;
        if (record->attr[i].type == SDP_TYPE_UUID16 &&
            uuid && uuid->len == 2 &&
            record->attr[i].value.uuid.b[0] == uuid->b[0] &&
            record->attr[i].value.uuid.b[1] == uuid->b[1])
            return true;
    }
    return false;
}

static struct bt_buf *response_begin(u8 pdu_id, u16 tid)
{
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return NULL;

    u8 hdr[5] = { pdu_id, (u8)(tid & 0xFF), (u8)(tid >> 8), 0, 0 };
    bt_buf_append(buf, hdr, sizeof(hdr));
    return buf;
}

static void response_finish(struct sdp_server *server, struct bt_buf *buf)
{
    if (!buf) {
        return;
    }
    u16 len = (u16)(bt_buf_len(buf) - 5);
    buf->data[3] = (u8)(len & 0xFF);
    buf->data[4] = (u8)(len >> 8);
    if (server->chan)
        l2cap_send(server->chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

static void handle_search(struct sdp_server *server, u16 tid,
                          const u8 *data, u16 len)
{
    bt_uuid uuid = { .len = 0, .b = { 0 } };

    if (len >= 3 && data[0] == 0x35 && data[1] == 0x03 && data[2] == 0x19) {
        uuid.len = 2;
        uuid.b[0] = data[4];
        uuid.b[1] = data[5];
    }

    u8 count = 0;
    for (unsigned int i = 0; i < server->count; i++) {
        if (!uuid.len || record_matches(server->record[i], &uuid))
            count++;
    }

    struct bt_buf *rsp = response_begin(SDP_PDU_SEARCH_RSP, tid);
    if (!rsp)
        return;

    bt_buf_append(rsp, &count, 1);
    bt_buf_append(rsp, &count, 1);

    for (unsigned int i = 0; i < server->count; i++) {
        if (uuid.len && !record_matches(server->record[i], &uuid))
            continue;
        u32 handle = server->record[i]->handle;
        u8 raw[4] = { (u8)handle, (u8)(handle >> 8), (u8)(handle >> 16), (u8)(handle >> 24) };
        bt_buf_append(rsp, raw, sizeof(raw));
    }

    u16 zero = 0;
    bt_buf_append(rsp, &zero, sizeof(zero));
    response_finish(server, rsp);
}

static void handle_get_attrs(struct sdp_server *server, u16 tid,
                             const u8 *data, u16 len)
{
    u32 handle = 0;
    if (len >= 6)
        handle = (u32)data[1] | ((u32)data[2] << 8) |
                 ((u32)data[3] << 16) | ((u32)data[4] << 24);

    struct bt_buf *rsp = response_begin(SDP_PDU_ATTR_RSP, tid);
    if (!rsp)
        return;

    u16 attr_bytes = 0;
    bt_buf_append(rsp, &attr_bytes, 2);
    response_finish(server, rsp);
    (void)handle;
}

void sdp_server_rx(struct sdp_server *server, struct bt_buf *buf)
{
    if (!server || !buf)
        return;

    u8 pdu_id = bt_buf_get_u8(buf);
    u16 tid = bt_buf_get_le16(buf);
    u16 len = bt_buf_get_le16(buf);
    const u8 *data = bt_buf_pull(buf, len);

    if (!data && len)
        return;

    switch (pdu_id) {
    case SDP_PDU_SEARCH_REQ:
        handle_search(server, tid, data, len);
        break;
    case SDP_PDU_ATTR_REQ:
        handle_get_attrs(server, tid, data, len);
        break;
    default:
        break;
    }

    bt_buf_put(buf);
}

int sdp_client_search(struct l2cap_chan *chan, const bt_uuid *uuid,
                      u32 *handles, u8 max_handles)
{
    struct bt_buf *buf;
    u8 pdu[16];

    if (!chan || !uuid)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    u8 hdr[5] = { SDP_PDU_SEARCH_REQ, 0x01, 0x00, 0x00, 0x00 };
    bt_buf_append(buf, hdr, sizeof(hdr));

    u8 seq[] = { 0x35, 0x03, 0x19, uuid->b[0], uuid->b[1] };
    bt_buf_append(buf, seq, sizeof(seq));
    bt_buf_append(buf, (u8[]){ 0x00 }, 1);

    u16 total = (u16)(bt_buf_len(buf) - 5);
    buf->data[3] = (u8)(total & 0xFF);
    buf->data[4] = (u8)(total >> 8);

    int ret = l2cap_send(chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    (void)pdu;
    (void)handles;
    (void)max_handles;
    return ret;
}

int sdp_client_get_attrs(struct l2cap_chan *chan, u32 handle, u16 attr_id)
{
    struct bt_buf *buf;
    u8 hdr[5] = { SDP_PDU_ATTR_REQ, 0x02, 0x00, 0x00, 0x00 };
    u8 req[8];

    if (!chan)
        return -1;

    buf = bt_buf_alloc(0);
    if (!buf)
        return -2;

    bt_buf_append(buf, hdr, sizeof(hdr));

    req[0] = 0x00;
    req[1] = (u8)handle;
    req[2] = (u8)(handle >> 8);
    req[3] = (u8)(handle >> 16);
    req[4] = (u8)(handle >> 24);
    req[5] = 0x00;
    req[6] = 0x00;
    req[7] = 0x00;
    bt_buf_append(buf, req, sizeof(req));

    u16 total = (u16)(bt_buf_len(buf) - 5);
    buf->data[3] = (u8)(total & 0xFF);
    buf->data[4] = (u8)(total >> 8);

    int ret = l2cap_send(chan, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
    (void)attr_id;
    return ret;
}
