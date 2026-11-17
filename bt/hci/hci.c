#include <bt/hci.h>
#include <bt/stack.h>
#include <core/string.h>
#include <core/printk.h>

#define HCI_MAX_CONNS 8
#define HCI_MAX_PENDING 8

struct pending_cmd {
    u16 opcode;
    hci_cmd_cb_t cb;
    void *arg;
    struct pending_cmd *next;
};

struct event_slot {
    hci_event_cb_t cb;
    void *arg;
};

struct conn_slot {
    struct hci_conn conn;
    struct bt_buf *rx;
};

static struct {
    const struct bt_transport *transport;
    bool opened;
    bool ready;
    bt_addr local;
    struct pending_cmd *pending;
    struct pending_cmd pending_pool[HCI_MAX_PENDING];
    u8 pending_used;
    struct event_slot events[64];
    struct event_slot le_events[16];
    hci_acl_cb_t acl_cb;
    struct conn_slot conns[HCI_MAX_CONNS];
} hci;

static struct pending_cmd *pending_add(u16 opcode, hci_cmd_cb_t cb, void *arg)
{
    if (!cb || hci.pending_used >= HCI_MAX_PENDING)
        return NULL;

    struct pending_cmd *cmd = &hci.pending_pool[hci.pending_used++];
    cmd->opcode = opcode;
    cmd->cb = cb;
    cmd->arg = arg;
    cmd->next = hci.pending;
    hci.pending = cmd;
    return cmd;
}

static struct pending_cmd *pending_take(u16 opcode)
{
    struct pending_cmd **at = &hci.pending;

    while (*at) {
        if ((*at)->opcode == opcode) {
            struct pending_cmd *cmd = *at;
            *at = cmd->next;
            return cmd;
        }
        at = &(*at)->next;
    }
    return NULL;
}

int bt_transport_register(const struct bt_transport *transport)
{
    if (!transport || !transport->send)
        return -1;
    hci.transport = transport;
    return 0;
}

const char *bt_transport_name(void)
{
    return hci.transport ? hci.transport->name : "(none)";
}

int hci_init(void)
{
    memset(&hci, 0, sizeof(hci));
    bt_buf_init();
    return 0;
}

int hci_open(void)
{
    if (!hci.transport)
        return -1;
    if (hci.opened)
        return 0;

    if (hci.transport->open) {
        int ret = hci.transport->open();
        if (ret)
            return ret;
    }

    hci.opened = true;
    hci_send_cmd(HCI_OP_RESET, NULL, 0, NULL, NULL);
    hci_send_cmd(HCI_OP_READ_BD_ADDR, NULL, 0, NULL, NULL);
    return 0;
}

int hci_close(void)
{
    if (!hci.opened)
        return 0;
    if (hci.transport && hci.transport->close)
        hci.transport->close();
    hci.opened = false;
    hci.ready = false;
    return 0;
}

bool hci_ready(void)
{
    return hci.ready;
}

int hci_send_cmd(u16 opcode, const void *params, u8 len,
                 hci_cmd_cb_t cb, void *arg)
{
    u8 packet[4 + 255];

    if (!hci.transport || !hci.opened)
        return -1;

    packet[0] = 0x01;
    packet[1] = (u8)(opcode & 0xFF);
    packet[2] = (u8)(opcode >> 8);
    packet[3] = len;
    if (len && params)
        memcpy(&packet[4], params, len);

    pending_add(opcode, cb, arg);
    return hci.transport->send(packet, (u16)(len + 4));
}

int hci_send_acl(u16 handle, u8 pb, const void *data, u16 len)
{
    u8 header[5];

    if (!hci.transport || !hci.opened)
        return -1;
    if (len > 0x0FFF)
        return -2;

    header[0] = 0x02;
    header[1] = (u8)(handle & 0xFF);
    header[2] = (u8)(((handle >> 8) & 0x0F) | (pb << 4));
    header[3] = (u8)(len & 0xFF);
    header[4] = (u8)(len >> 8);

    int ret = hci.transport->send(header, sizeof(header));
    if (ret)
        return ret;
    return data && len ? hci.transport->send(data, len) : 0;
}

void hci_register_event(u8 event, hci_event_cb_t cb, void *arg)
{
    if (event >= 64)
        return;
    hci.events[event].cb = cb;
    hci.events[event].arg = arg;
}

void hci_register_le(u8 subevent, hci_event_cb_t cb, void *arg)
{
    if (subevent >= 16)
        return;
    hci.le_events[subevent].cb = cb;
    hci.le_events[subevent].arg = arg;
}

void hci_register_acl(hci_acl_cb_t cb)
{
    hci.acl_cb = cb;
}

struct hci_conn *hci_conn_alloc(u16 handle, const bt_addr *addr,
                                enum bt_link_type type)
{
    for (unsigned int i = 0; i < HCI_MAX_CONNS; i++) {
        struct conn_slot *slot = &hci.conns[i];
        if (slot->conn.used)
            continue;

        slot->conn.used = true;
        slot->conn.handle = handle;
        slot->conn.peer = *addr;
        slot->conn.type = type;
        slot->conn.encrypted = false;
        slot->rx = NULL;
        return &slot->conn;
    }
    return NULL;
}

void hci_conn_free(u16 handle)
{
    for (unsigned int i = 0; i < HCI_MAX_CONNS; i++) {
        struct conn_slot *slot = &hci.conns[i];
        if (!slot->conn.used || slot->conn.handle != handle)
            continue;
        if (slot->rx)
            bt_buf_put(slot->rx);
        slot->rx = NULL;
        memset(&slot->conn, 0, sizeof(slot->conn));
        return;
    }
}

struct hci_conn *hci_conn_find(u16 handle)
{
    for (unsigned int i = 0; i < HCI_MAX_CONNS; i++) {
        if (hci.conns[i].conn.used && hci.conns[i].conn.handle == handle)
            return &hci.conns[i].conn;
    }
    return NULL;
}

struct hci_conn *hci_conn_find_addr(const bt_addr *addr)
{
    for (unsigned int i = 0; i < HCI_MAX_CONNS; i++) {
        if (hci.conns[i].conn.used &&
            bt_addr_eq(&hci.conns[i].conn.peer, addr))
            return &hci.conns[i].conn;
    }
    return NULL;
}

const bt_addr *hci_local_addr(void)
{
    return &hci.local;
}

static void handle_cmd_complete(const u8 *data, u16 len)
{
    struct bt_buf *params;
    struct pending_cmd *cmd;
    u8 status;
    u16 opcode;

    if (len < 3)
        return;

    status = data[0];
    opcode = (u16)(data[1] | (data[2] << 8));

    cmd = pending_take(opcode);
    if (!cmd)
        return;

    if (opcode == HCI_OP_READ_BD_ADDR && status == BT_OK && len >= 9) {
        for (int i = 0; i < 6; i++)
            hci.local.b[i] = data[3 + i];
        hci.ready = true;
        bt_publish(BT_EVT_READY, &hci.local);
    }

    if (cmd->cb) {
        params = bt_buf_alloc(0);
        if (params && len > 3)
            bt_buf_append(params, data + 3, (u16)(len - 3));
        cmd->cb(status, params, cmd->arg);
        if (params)
            bt_buf_put(params);
    }
}

static void handle_cmd_status(const u8 *data, u16 len)
{
    struct pending_cmd *cmd;
    u8 status;
    u16 opcode;

    if (len < 4)
        return;

    status = data[0];
    opcode = (u16)(data[2] | (data[3] << 8));
    cmd = pending_take(opcode);
    if (cmd && cmd->cb)
        cmd->cb(status, NULL, cmd->arg);
}

static void handle_conn_complete(const u8 *data, u16 len, bool le)
{
    struct hci_conn *conn;
    bt_addr peer;
    u16 handle;

    if (len < 11)
        return;

    u8 status = le ? data[0] : data[0];
    handle = (u16)(data[1] | (data[2] << 8));
    for (int i = 0; i < 6; i++)
        peer.b[i] = data[3 + i];

    if (status != BT_OK)
        return;

    enum bt_link_type type = le ? BT_LINK_LE : BT_LINK_BR_EDR;
    conn = hci_conn_find(handle);
    if (!conn)
        conn = hci_conn_alloc(handle, &peer, type);

    if (hci.events[HCI_EV_CONN_COMPLETE].cb)
        hci.events[HCI_EV_CONN_COMPLETE].cb(data, len,
                                            hci.events[HCI_EV_CONN_COMPLETE].arg);

    bt_publish(BT_EVT_CONNECTED, conn);
}

static void handle_disconn_complete(const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u8 status = data[0];
    u16 handle = (u16)(data[1] | (data[2] << 8));
    u8 reason = data[3];

    if (status == BT_OK) {
        struct hci_conn *conn = hci_conn_find(handle);
        if (conn)
            bt_publish(BT_EVT_DISCONNECTED, conn);
        hci_conn_free(handle);
        bt_publish(BT_EVT_DISCONNECTED, &reason);
    }
}

static void handle_acl(const u8 *data, u16 len)
{
    if (len < 4)
        return;

    u16 handle = (u16)((data[0] | (data[1] << 8)) & HCI_ACL_HANDLE_MASK);
    u8 pb = (u8)((data[1] >> 4) & 0x03);
    u16 dlen = (u16)(data[2] | (data[3] << 8));
    const u8 *payload = data + 4;

    if (4 + dlen > len)
        dlen = (u16)(len - 4);

    struct conn_slot *slot = NULL;
    for (unsigned int i = 0; i < HCI_MAX_CONNS; i++) {
        if (hci.conns[i].conn.used && hci.conns[i].conn.handle == handle) {
            slot = &hci.conns[i];
            break;
        }
    }
    if (!slot)
        return;

    bool continuation = (pb & 0x01) != 0;

    if (!continuation) {
        if (slot->rx)
            bt_buf_put(slot->rx);
        slot->rx = bt_buf_alloc(0);
        if (!slot->rx)
            return;
    }

    if (!slot->rx)
        slot->rx = bt_buf_alloc(0);
    if (!slot->rx)
        return;

    if (bt_buf_append(slot->rx, payload, dlen) != dlen) {
        bt_buf_put(slot->rx);
        slot->rx = NULL;
        return;
    }

    if (continuation)
        return;

    struct bt_buf *complete = slot->rx;
    slot->rx = NULL;
    complete->flags |= BT_BUF_F_ACL;

    if (hci.acl_cb)
        hci.acl_cb(handle, pb, complete);
    else
        bt_buf_put(complete);
}

void hci_rx(const u8 *data, u16 len)
{
    if (!data || !len)
        return;

    switch (data[0]) {
    case 0x04: {
        if (len < 2)
            return;
        u8 event = data[1];
        const u8 *body = data + 2;
        u16 body_len = (u16)(len - 2);

        if (event == HCI_EV_CMD_COMPLETE) {
            handle_cmd_complete(body, body_len);
        } else if (event == HCI_EV_CMD_STATUS) {
            handle_cmd_status(body, body_len);
        } else if (event == HCI_EV_CONN_COMPLETE) {
            handle_conn_complete(body, body_len, false);
        } else if (event == HCI_EV_DISCONN_COMPLETE) {
            handle_disconn_complete(body, body_len);
        } else if (event == HCI_EV_LE_META) {
            if (body_len >= 1) {
                u8 sub = body[0];
                if (sub == HCI_LE_CONN_COMPLETE)
                    handle_conn_complete(body + 1, (u16)(body_len - 1), true);
                if (sub < 16 && hci.le_events[sub].cb)
                    hci.le_events[sub].cb(body, body_len,
                                           hci.le_events[sub].arg);
            }
        } else {
            if (event < 64 && hci.events[event].cb)
                hci.events[event].cb(body, body_len, hci.events[event].arg);
        }
        break;
    }
    case 0x02:
        handle_acl(data + 1, (u16)(len - 1));
        break;
    default:
        break;
    }
}

void hci_poll(void)
{
    if (!hci.transport || !hci.transport->read)
        return;

    u8 scratch[64];
    u16 n = hci.transport->read(scratch, sizeof(scratch));
    if (n)
        hci_rx(scratch, n);
}
