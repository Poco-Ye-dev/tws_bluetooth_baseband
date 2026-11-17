#ifndef __BT_SDP_H__
#define __BT_SDP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/l2cap.h>
#include <bt/buf.h>

/*
 * Service Discovery Protocol.
 *
 * A record is a list of attributes; the encoder writes the standard data
 * element sequence so a remote SDP client can search by UUID and fetch the
 * protocol descriptor list.
 */

#define SDP_MAX_RECORDS 8
#define SDP_MAX_ATTRS   12

#define SDP_ATTR_SERVICE_RECORD_HANDLE 0x0000
#define SDP_ATTR_SERVICE_CLASS_IDS     0x0001
#define SDP_ATTR_PROTOCOL_DESC_LIST    0x0004
#define SDP_ATTR_SERVICE_NAME          0x0100

#define SDP_PSM 0x0001

enum sdp_type {
    SDP_TYPE_U8 = 0,
    SDP_TYPE_U16,
    SDP_TYPE_U32,
    SDP_TYPE_UUID16,
    SDP_TYPE_UUID128,
    SDP_TYPE_STRING,
    SDP_TYPE_SEQUENCE,
};

struct sdp_attr {
    u16 id;
    enum sdp_type type;
    union {
        u8 u8;
        u16 u16;
        u32 u32;
        bt_uuid uuid;
        const char *str;
    } value;
};

struct sdp_record {
    u32 handle;
    struct sdp_attr attr[SDP_MAX_ATTRS];
    u8 count;
};

struct sdp_server {
    struct l2cap_chan *chan;
    struct sdp_record *record[SDP_MAX_RECORDS];
    u8 count;
};

int sdp_server_init(struct sdp_server *server);
int sdp_register_record(struct sdp_server *server, struct sdp_record *record);
u32 sdp_next_handle(void);
void sdp_server_rx(struct sdp_server *server, struct bt_buf *buf);

/* Client */
int sdp_client_search(struct l2cap_chan *chan, const bt_uuid *uuid,
                      u32 *handles, u8 max_handles);
int sdp_client_get_attrs(struct l2cap_chan *chan, u32 handle,
                         u16 attr_id);

#endif /* __BT_SDP_H__ */
