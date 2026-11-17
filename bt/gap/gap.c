#include <bt/gap.h>
#include <bt/hci.h>
#include <bt/stack.h>
#include <core/string.h>
#include <core/printk.h>

static struct {
    struct gap_adv_config adv_config;
    u8 adv_data[GAP_ADV_DATA_MAX];
    u8 adv_len;
    u8 scan_rsp[GAP_ADV_DATA_MAX];
    u8 scan_rsp_len;
    bool adv_enabled;
    bool scan_enabled;
    struct gap_callbacks cb;
} gap;

void gap_init(const struct gap_callbacks *cb)
{
    memset(&gap, 0, sizeof(gap));
    if (cb)
        gap.cb = *cb;

    gap.adv_config.type = GAP_ADV_IND;
    gap.adv_config.interval_min = 0x00A0;
    gap.adv_config.interval_max = 0x00F0;
    gap.adv_config.channel_map = 0x07;
}

void adv_builder_reset(struct adv_builder *b)
{
    b->len = 0;
    b->overflow = false;
}

int adv_builder_add(struct adv_builder *b, u8 type, const void *data, u8 len)
{
    if (!b)
        return -1;
    if (b->len + len + 2 > GAP_ADV_DATA_MAX) {
        b->overflow = true;
        return -2;
    }

    b->data[b->len++] = (u8)(len + 1);
    b->data[b->len++] = type;
    if (len && data)
        memcpy(&b->data[b->len], data, len);
    b->len += len;
    return 0;
}

int adv_builder_add_flags(struct adv_builder *b, u8 flags)
{
    return adv_builder_add(b, 0x01, &flags, 1);
}

int adv_builder_add_name(struct adv_builder *b, const char *name)
{
    size_t len = strlen(name);
    if (len > 20)
        len = 20;
    return adv_builder_add(b, 0x09, name, (u8)len);
}

int adv_builder_add_uuid16(struct adv_builder *b, u16 uuid)
{
    u8 raw[2] = { (u8)(uuid & 0xFF), (u8)(uuid >> 8) };
    return adv_builder_add(b, 0x03, raw, sizeof(raw));
}

int adv_builder_add_tx_power(struct adv_builder *b, s8 dbm)
{
    return adv_builder_add(b, 0x0A, &dbm, 1);
}

static void adv_complete(u8 status, struct bt_buf *params, void *arg)
{
    (void)status;
    (void)params;
    (void)arg;
}

int gap_set_adv_config(const struct gap_adv_config *config)
{
    u8 params[15];

    if (!config)
        return -1;

    gap.adv_config = *config;

    params[0] = (u8)(config->interval_min & 0xFF);
    params[1] = (u8)(config->interval_min >> 8);
    params[2] = (u8)(config->interval_max & 0xFF);
    params[3] = (u8)(config->interval_max >> 8);
    params[4] = (u8)config->type;
    params[5] = 0x00;               /* own address: public */
    params[6] = 0x00;               /* peer address type */
    for (int i = 0; i < 6; i++)
        params[7 + i] = config->direct_peer.b[i];
    params[13] = config->channel_map;
    params[14] = config->filter_policy;

    return hci_send_cmd(HCI_OP_LE_SET_ADV_PARAMS, params, sizeof(params),
                        adv_complete, NULL);
}

int gap_set_adv_data(const struct adv_builder *b)
{
    u8 params[32];

    if (!b || b->overflow)
        return -1;

    memset(params, 0, sizeof(params));
    memcpy(params, b->data, b->len);
    gap.adv_len = b->len;
    memcpy(gap.adv_data, b->data, b->len);

    return hci_send_cmd(HCI_OP_LE_SET_ADV_DATA, params, sizeof(params),
                        adv_complete, NULL);
}

int gap_set_scan_rsp(const struct adv_builder *b)
{
    u8 params[32];

    if (!b || b->overflow)
        return -1;

    memset(params, 0, sizeof(params));
    memcpy(params, b->data, b->len);
    gap.scan_rsp_len = b->len;
    memcpy(gap.scan_rsp, b->data, b->len);

    return hci_send_cmd(HCI_OP_LE_SET_SCAN_RSP_DATA, params, sizeof(params),
                        adv_complete, NULL);
}

int gap_start_adv(void)
{
    u8 enable = 0x01;
    int ret = hci_send_cmd(HCI_OP_LE_SET_ADV_ENABLE, &enable, 1,
                           adv_complete, NULL);
    if (!ret)
        gap.adv_enabled = true;
    return ret;
}

int gap_stop_adv(void)
{
    u8 enable = 0x00;
    int ret = hci_send_cmd(HCI_OP_LE_SET_ADV_ENABLE, &enable, 1,
                           adv_complete, NULL);
    if (!ret)
        gap.adv_enabled = false;
    return ret;
}

int gap_start_scan(const struct gap_scan_config *config)
{
    u8 params[7];

    if (!config)
        return -1;

    params[0] = (u8)(config->active ? 0x01 : 0x00);
    params[1] = (u8)(config->interval & 0xFF);
    params[2] = (u8)(config->interval >> 8);
    params[3] = (u8)(config->window & 0xFF);
    params[4] = (u8)(config->window >> 8);
    params[5] = 0x00;
    params[6] = config->filter_policy;

    int ret = hci_send_cmd(HCI_OP_LE_SET_SCAN_PARAMS, params, sizeof(params),
                           NULL, NULL);
    if (ret)
        return ret;

    u8 enable[2] = { 0x01, 0x00 };
    ret = hci_send_cmd(HCI_OP_LE_SET_SCAN_ENABLE, enable, sizeof(enable),
                       NULL, NULL);
    if (!ret)
        gap.scan_enabled = true;
    return ret;
}

int gap_stop_scan(void)
{
    u8 enable[2] = { 0x00, 0x00 };
    int ret = hci_send_cmd(HCI_OP_LE_SET_SCAN_ENABLE, enable, sizeof(enable),
                           NULL, NULL);
    if (!ret)
        gap.scan_enabled = false;
    return ret;
}

int gap_connect(const bt_addr *peer)
{
    u8 params[25];

    if (!peer)
        return -1;

    memset(params, 0, sizeof(params));
    params[0] = 0x00;               /* scan interval */
    params[1] = 0x00;
    params[2] = 0x00;
    params[3] = 0x00;
    params[4] = 0x00;               /* filter policy */
    params[5] = 0x00;               /* peer address type */
    for (int i = 0; i < 6; i++)
        params[6 + i] = peer->b[i];
    params[12] = 0x00;

    return hci_send_cmd(HCI_OP_LE_CREATE_CONN, params, sizeof(params),
                        NULL, NULL);
}

int gap_disconnect(u16 handle, u8 reason)
{
    u8 params[3] = {
        (u8)(handle & 0xFF), (u8)(handle >> 8), reason ? reason : 0x13,
    };
    return hci_send_cmd(HCI_OP_DISCONNECT, params, sizeof(params), NULL, NULL);
}

bool gap_adv_active(void)
{
    return gap.adv_enabled;
}

bool gap_scan_active(void)
{
    return gap.scan_enabled;
}

static void le_adv_report(const u8 *data, u16 len, void *arg)
{
    (void)arg;

    if (len < 10 || !gap.cb.adv_report)
        return;

    bt_addr peer;
    for (int i = 0; i < 6; i++)
        peer.b[i] = data[3 + i];

    u8 dlen = data[9];
    if (10 + dlen + 1 > len)
        return;

    s8 rssi = (s8)data[10 + dlen];
    gap.cb.adv_report(&peer, rssi, &data[10], dlen);
    bt_publish(BT_EVT_ADV_REPORT, &peer);
}

static void le_conn_complete(const u8 *data, u16 len, void *arg)
{
    (void)arg;

    if (len < 19 || data[0] != BT_OK)
        return;

    bt_addr peer;
    for (int i = 0; i < 6; i++)
        peer.b[i] = data[5 + i];

    u16 handle = (u16)(data[1] | (data[2] << 8));
    if (gap.cb.connected)
        gap.cb.connected(&peer, BT_LINK_LE, handle);
}

static void br_conn_complete(const u8 *data, u16 len, void *arg)
{
    (void)arg;

    if (len < 11 || data[0] != BT_OK)
        return;

    bt_addr peer;
    for (int i = 0; i < 6; i++)
        peer.b[i] = data[3 + i];

    u16 handle = (u16)(data[1] | (data[2] << 8));
    if (gap.cb.connected)
        gap.cb.connected(&peer, BT_LINK_BR_EDR, handle);
}

static void link_disconnected(const u8 *data, u16 len, void *arg)
{
    (void)arg;

    if (len < 4 || data[0] != BT_OK)
        return;

    struct hci_conn *conn = hci_conn_find((u16)(data[1] | (data[2] << 8)));
    if (conn && gap.cb.disconnected)
        gap.cb.disconnected(&conn->peer, data[3]);
}

void gap_register_handlers(void)
{
    hci_register_le(HCI_LE_ADV_REPORT, le_adv_report, NULL);
    hci_register_le(HCI_LE_CONN_COMPLETE, le_conn_complete, NULL);
    hci_register_event(HCI_EV_CONN_COMPLETE, br_conn_complete, NULL);
    hci_register_event(HCI_EV_DISCONN_COMPLETE, link_disconnected, NULL);
}
