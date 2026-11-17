#include "bb_internal.h"
#include <bt/baseband.h>
#include <bt/hci.h>
#include <bt/radio.h>
#include <core/string.h>
#include <core/errno.h>

/*
 * HCI <-> baseband bridge.
 *
 * Presents the link controller as an HCI transport: host commands are parsed
 * here, baseband events are converted back into HCI events and injected with
 * hci_rx().  This is what lets the stack run without an external controller.
 */

static bool le_connecting;
static u16 acl_handle;
static u32 slot_counter;

static void emit_event(u8 event, const u8 *data, u8 len)
{
    u8 packet[300];

    packet[0] = 0x04;
    packet[1] = event;
    packet[2] = len;
    if (len && data)
        memcpy(&packet[3], data, len);
    hci_rx(packet, (u16)(len + 3));
}

static void command_complete(u16 opcode, u8 status,
                             const u8 *params, u8 len)
{
    u8 payload[64];

    payload[0] = status;
    payload[1] = (u8)(opcode & 0xFF);
    payload[2] = (u8)(opcode >> 8);
    if (len && params)
        memcpy(&payload[3], params, len);

    emit_event(HCI_EV_CMD_COMPLETE, payload, (u8)(len + 3));
}

static void on_link_up(struct bb_link *link)
{
    if (le_connecting) {
        u8 payload[19];
        payload[0] = 0x01;              /* LE connection complete */
        payload[1] = 0x00;              /* status */
        payload[2] = (u8)(link->handle & 0xFF);
        payload[3] = (u8)(link->handle >> 8);
        payload[4] = 0x00;              /* role: central */
        payload[5] = 0x00;              /* peer address type */
        for (int i = 0; i < 6; i++)
            payload[6 + i] = link->peer.b[i];
        payload[12] = 0x06;             /* interval */
        payload[13] = 0x00;
        payload[14] = 0x00;
        payload[15] = 0x00;
        payload[16] = 0x0A;             /* supervision timeout */
        payload[17] = 0x00;
        payload[18] = 0x00;
        emit_event(HCI_EV_LE_META, payload, sizeof(payload));
        le_connecting = false;
    } else {
        u8 payload[11];
        payload[0] = 0x00;              /* status */
        payload[1] = (u8)(link->handle & 0xFF);
        payload[2] = (u8)(link->handle >> 8);
        for (int i = 0; i < 6; i++)
            payload[3 + i] = link->peer.b[i];
        payload[9] = 0x01;              /* ACL link */
        payload[10] = 0x00;             /* encryption off */
        emit_event(HCI_EV_CONN_COMPLETE, payload, sizeof(payload));
    }

    acl_handle = link->handle;
}

static void on_link_down(struct bb_link *link, u8 reason)
{
    u8 payload[4] = {
        0x00,
        (u8)(link->handle & 0xFF),
        (u8)(link->handle >> 8),
        reason ? reason : 0x13,
    };
    emit_event(HCI_EV_DISCONN_COMPLETE, payload, sizeof(payload));
}

static void on_acl_rx(struct bb_link *link, const u8 *data, u16 len)
{
    u8 packet[4 + BB_MAX_ACL_MTU];

    packet[0] = 0x02;
    packet[1] = (u8)(link->handle & 0xFF);
    packet[2] = (u8)(((link->handle >> 8) & 0x0F) | 0x20);
    packet[3] = (u8)(len & 0xFF);
    packet[4] = (u8)(len >> 8);
    memcpy(&packet[5], data, len);

    hci_rx(packet, (u16)(len + 5));
}

static void on_inquiry(const bt_addr *addr, s8 rssi)
{
    u8 payload[14];

    payload[0] = 0x01;                  /* inquiry result */
    payload[1] = 0x01;                  /* one response */
    for (int i = 0; i < 6; i++)
        payload[2 + i] = addr->b[i];
    payload[8] = 0x01;
    payload[9] = 0x00;
    payload[10] = 0x00;
    payload[11] = (u8)rssi;
    payload[12] = 0x00;
    payload[13] = 0x00;
    emit_event(0x02, payload, sizeof(payload));
}

static const struct bb_ops bb_bridge_ops = {
    .link_up = on_link_up,
    .link_down = on_link_down,
    .acl_rx = on_acl_rx,
    .inquiry_result = on_inquiry,
};

static void handle_command(u16 opcode, const u8 *params, u8 len)
{
    switch (opcode) {
    case HCI_OP_RESET:
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_READ_BD_ADDR: {
        const bt_addr *addr = bb_local_addr();
        command_complete(opcode, 0x00, addr->b, 6);
        break;
    }

    case HCI_OP_LE_SET_ADV_PARAMS:
        if (len >= 6) {
            u16 interval = (u16)(params[0] | (params[1] << 8));
            bb_le_set_adv_params(interval, interval, params[4]);
        }
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_LE_SET_ADV_DATA:
        if (len >= 2)
            bb_le_set_adv_data(&params[1], params[0]);
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_LE_SET_SCAN_RSP_DATA:
        if (len >= 2)
            bb_le_set_scan_rsp(&params[1], params[0]);
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_LE_SET_ADV_ENABLE:
        if (len >= 1)
            bb_le_adv_enable(params[0] != 0);
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_LE_CREATE_CONN:
        if (len >= 12) {
            bt_addr peer;
            for (int i = 0; i < 6; i++)
                peer.b[i] = params[6 + i];
            le_connecting = true;
            bb_le_start_connect(&peer);
        }
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_CREATE_CONN:
        if (len >= 6) {
            bt_addr peer;
            for (int i = 0; i < 6; i++)
                peer.b[i] = params[i];
            bb_connect(&peer);
        }
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_DISCONNECT:
        if (len >= 2) {
            u16 handle = (u16)(params[0] | (params[1] << 8));
            bb_disconnect(handle, len >= 3 ? params[2] : 0x13);
        }
        command_complete(opcode, 0x00, NULL, 0);
        break;

    case HCI_OP_LE_SET_SCAN_ENABLE:
        bb_set_scan(false, len >= 1 && params[0]);
        command_complete(opcode, 0x00, NULL, 0);
        break;

    default:
        command_complete(opcode, 0x01, NULL, 0);
        break;
    }
}

static int bb_transport_send(const u8 *data, u16 len)
{
    if (!data || len < 1)
        return -EINVAL;

    if (data[0] == 0x01 && len >= 4) {
        u16 opcode = (u16)(data[1] | (data[2] << 8));
        u8 plen = data[3];
        if ((u16)(plen + 4) > len)
            plen = (u8)(len - 4);
        handle_command(opcode, &data[4], plen);
        return 0;
    }

    if (data[0] == 0x02 && len >= 5) {
        u16 handle = (u16)((data[1] | (data[2] << 8)) & HCI_ACL_HANDLE_MASK);
        u16 plen = (u16)(data[3] | (data[4] << 8));
        return bb_send_acl(handle, &data[5], plen);
    }

    return -EPROTO;
}

static const struct bt_transport bb_transport = {
    .name = "baseband",
    .send = bb_transport_send,
};

int bt_baseband_init(const struct bb_config *config)
{
    if (!config)
        return -EINVAL;

    bt_transport_register(&bb_transport);
    int ret = bb_init(config, &bb_bridge_ops);
    if (ret)
        return ret;

    acl_handle = 0;
    le_connecting = false;
    slot_counter = 0;
    return bb_start();
}

void bt_baseband_tick(void)
{
    bb_tick(++slot_counter);
}
