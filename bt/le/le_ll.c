#include <bt/le_ll.h>
#include <bt/le_csa.h>
#include <bt/baseband.h>
#include <core/string.h>
#include <core/printk.h>

/*
 * LE connection protocol engine.
 *
 * Only one PDU is kept outstanding per connection, which matches the LE
 * retransmission model: the radio sends the pending PDU on every connection
 * event until its NESN acknowledges it.
 */

static struct le_ll_conn conns[LE_LL_MAX_CONN];
static struct le_ll_ops ll_ops;

void le_ll_init(const struct le_ll_ops *ops)
{
    memset(conns, 0, sizeof(conns));
    if (ops)
        ll_ops = *ops;
}

struct le_ll_conn *le_ll_find(u16 handle)
{
    for (unsigned int i = 0; i < LE_LL_MAX_CONN; i++) {
        if (conns[i].used && conns[i].handle == handle)
            return &conns[i];
    }
    return NULL;
}

struct le_ll_conn *le_ll_create(u16 handle, const bt_addr *peer,
                                u32 access_address, u32 crc_init,
                                u16 interval, u16 latency, u16 timeout)
{
    for (unsigned int i = 0; i < LE_LL_MAX_CONN; i++) {
        struct le_ll_conn *conn = &conns[i];
        if (conn->used)
            continue;

        memset(conn, 0, sizeof(*conn));
        conn->used = true;
        conn->handle = handle;
        conn->peer = *peer;
        conn->access_address = access_address;
        conn->crc_init = crc_init;
        conn->interval = interval ? interval : 6;
        conn->latency = latency;
        conn->timeout = timeout ? timeout : 500;
        conn->state = LE_LL_CONNECTED;
        memset(conn->channel_map, 0xFF, sizeof(conn->channel_map));
        conn->sn = 0;
        conn->nesn = 1;
        conn->next_event_slot = bb_slots() + (u32)conn->interval * 16 / 10;
        conn->last_rx_slot = bb_slots();
        if (ll_ops.connected)
            ll_ops.connected(conn);
        return conn;
    }
    return NULL;
}

static void build_header(u8 *hdr, u8 llid, u8 length)
{
    hdr[0] = (u8)((llid & 0x03) | ((0 & 1) << 2) | ((0 & 1) << 3) |
                  (0 << 4) | (0 << 5));
    hdr[1] = length;
}

static void build_nonce(struct le_ll_conn *conn, u8 nonce[13])
{
    u32 counter = conn->packet_counter;

    nonce[0] = (u8)(counter & 0xFF);
    nonce[1] = (u8)((counter >> 8) & 0xFF);
    nonce[2] = (u8)((counter >> 16) & 0xFF);
    nonce[3] = (u8)((counter >> 24) & 0xFF);
    nonce[4] = 0;
    nonce[5] = 0;                   /* direction: 0 = master to slave */
    memcpy(&nonce[5], conn->iv, 8);
}

static int transmit(struct le_ll_conn *conn, u8 llid, const u8 *data, u8 len)
{
    u8 pdu[LE_LL_MAX_PDU + 4];
    u8 hdr[2];
    u8 cipher[LE_LL_MAX_PDU + 4];

    build_header(hdr, llid, (u8)(len + (conn->encrypted ? 4 : 0)));
    hdr[0] |= (u8)(((conn->nesn & 1) << 2) | ((conn->sn & 1) << 3));
    memcpy(pdu, hdr, 2);

    if (conn->encrypted) {
        u8 nonce[13];
        build_nonce(conn, nonce);
        aes_ccm_encrypt(conn->sk, nonce, data, len, &pdu[2]);
        conn->packet_counter++;
    } else if (len) {
        memcpy(&pdu[2], data, len);
    }

    conn->pending_len = (u16)(2 + len + (conn->encrypted ? 4 : 0));
    memcpy(conn->pending_tx, pdu, conn->pending_len);
    conn->pending_llid = llid;
    conn->pending_ack = true;

    /* The radio transmission itself is performed by the baseband slot task. */
    (void)cipher;
    conn->packets_tx++;
    return 0;
}

int le_ll_send(struct le_ll_conn *conn, const u8 *data, u16 len)
{
    if (!conn || !data || len > LE_LL_MAX_PDU)
        return -1;
    if (len > 27)
        return transmit(conn, LE_LLID_DATA_START, data, 27);
    return transmit(conn, LE_LLID_DATA_START, data, (u8)len);
}

int le_ll_send_control(struct le_ll_conn *conn, u8 opcode,
                       const u8 *payload, u8 len)
{
    u8 pdu[LE_LL_MAX_PDU];

    if (!conn || (size_t)len + 1 > sizeof(pdu))
        return -1;
    pdu[0] = opcode;
    if (len && payload)
        memcpy(&pdu[1], payload, len);
    return transmit(conn, LE_LLID_CONTROL, pdu, (u8)(len + 1));
}

int le_ll_update_params(struct le_ll_conn *conn, u16 interval, u16 latency,
                        u16 timeout)
{
    u8 payload[8];

    payload[0] = (u8)(interval & 0xFF);
    payload[1] = (u8)(interval >> 8);
    payload[2] = (u8)(latency & 0xFF);
    payload[3] = (u8)(latency >> 8);
    payload[4] = (u8)(timeout & 0xFF);
    payload[5] = (u8)(timeout >> 8);
    payload[6] = 0;
    payload[7] = 0;

    conn->interval = interval;
    conn->latency = latency;
    conn->timeout = timeout;
    return le_ll_send_control(conn, LE_LL_CONNECTION_UPDATE_IND, payload, 8);
}

int le_ll_update_channel_map(struct le_ll_conn *conn, const u8 map[5])
{
    if (!conn || !map)
        return -1;
    memcpy(conn->channel_map, map, sizeof(conn->channel_map));
    return le_ll_send_control(conn, LE_LL_CHANNEL_MAP_IND, map, 5);
}

int le_ll_start_encryption(struct le_ll_conn *conn,
                           const u8 rand[8], const u8 ediv[2],
                           const u8 ltk[16])
{
    u8 payload[16];

    if (!conn || !ltk)
        return -1;

    memcpy(payload, rand ? rand : conn->iv, 8);
    (void)ediv;
    memcpy(&payload[8], ltk, 8);

    memcpy(conn->ltk, ltk, 16);
    memcpy(conn->sk, ltk, 16);
    conn->state = LE_LL_ENCRYPTING;
    return le_ll_send_control(conn, LE_LL_ENC_REQ, payload, 16);
}

int le_ll_terminate(struct le_ll_conn *conn, u8 reason)
{
    u8 payload = reason;

    if (!conn)
        return -1;
    le_ll_send_control(conn, LE_LL_TERMINATE_IND, &payload, 1);
    conn->state = LE_LL_DISCONNECTING;
    return 0;
}

static void handle_control(struct le_ll_conn *conn, const u8 *data, u16 len)
{
    u8 opcode;

    if (!len)
        return;
    opcode = data[0];

    switch (opcode) {
    case LE_LL_CONNECTION_UPDATE_IND:
        if (len >= 9) {
            conn->interval = (u16)(data[1] | (data[2] << 8));
            conn->latency = (u16)(data[3] | (data[4] << 8));
            conn->timeout = (u16)(data[5] | (data[6] << 8));
        }
        break;
    case LE_LL_CHANNEL_MAP_IND:
        if (len >= 6)
            memcpy(conn->channel_map, &data[1], 5);
        break;
    case LE_LL_ENC_REQ:
        if (len >= 17) {
            memcpy(conn->iv, &data[1], 8);
            memcpy(conn->sk, &data[9], 16);
            le_ll_send_control(conn, LE_LL_ENC_RSP, &data[1], 8);
            conn->state = LE_LL_ENCRYPTING;
        }
        break;
    case LE_LL_ENC_RSP:
        le_ll_send_control(conn, LE_LL_START_ENC_REQ, NULL, 0);
        break;
    case LE_LL_START_ENC_REQ:
        le_ll_send_control(conn, LE_LL_START_ENC_RSP, NULL, 0);
        conn->state = LE_LL_ENCRYPTED;
        conn->encrypted = true;
        conn->packet_counter = 0;
        if (ll_ops.encrypted_changed)
            ll_ops.encrypted_changed(conn);
        break;
    case LE_LL_START_ENC_RSP:
        conn->state = LE_LL_ENCRYPTED;
        conn->encrypted = true;
        if (ll_ops.encrypted_changed)
            ll_ops.encrypted_changed(conn);
        break;
    case LE_LL_TERMINATE_IND:
        conn->state = LE_LL_DISCONNECTING;
        conn->used = false;
        if (ll_ops.disconnected)
            ll_ops.disconnected(conn);
        break;
    case LE_LL_PING_REQ:
        le_ll_send_control(conn, LE_LL_PING_RSP, NULL, 0);
        break;
    case LE_LL_FEATURE_REQ:
        le_ll_send_control(conn, LE_LL_FEATURE_RSP, conn->features,
                           LE_LL_FEATURE_LEN);
        break;
    case LE_LL_VERSION_IND:
    case LE_LL_LENGTH_REQ:
    case LE_LL_PHY_REQ:
    default:
        le_ll_send_control(conn, LE_LL_UNKNOWN_RSP, &opcode, 1);
        break;
    }
}

void le_ll_rx(struct le_ll_conn *conn, const u8 *pdu, u16 len)
{
    u8 hdr0, plen;
    u8 llid;
    u8 sn, nesn;
    const u8 *payload;
    u8 plain[LE_LL_MAX_PDU];

    if (!conn || !pdu || len < 2)
        return;

    hdr0 = pdu[0];
    plen = pdu[1];
    llid = hdr0 & 0x03;
    nesn = (hdr0 >> 2) & 1;
    sn = (hdr0 >> 3) & 1;

    if (len < 2 + plen)
        return;

    payload = &pdu[2];

    if (nesn == conn->tx_seq) {
        conn->pending_ack = false;
        conn->tx_seq ^= 1;
        conn->sn = conn->tx_seq;
    }

    if (sn != conn->nesn)
        return;

    conn->nesn ^= 1;
    conn->last_rx_slot = bb_slots();
    conn->packets_rx++;

    if (conn->encrypted) {
        u8 nonce[13];
        build_nonce(conn, nonce);
        if (plen < 4)
            return;
        if (aes_ccm_decrypt(conn->sk, nonce, payload, plen, plain))
            return;
        conn->packet_counter++;
        payload = plain;
        plen -= 4;
    }

    if (llid == LE_LLID_CONTROL) {
        handle_control(conn, payload, plen);
        return;
    }

    if (llid == LE_LLID_DATA_START) {
        conn->rx_len = plen;
        memcpy(conn->rx_assembly, payload, plen);
        if (ll_ops.data_rx)
            ll_ops.data_rx(conn, conn->rx_assembly, conn->rx_len);
    } else if (llid == LE_LLID_DATA_CONT) {
        if (conn->rx_len + plen <= sizeof(conn->rx_assembly)) {
            memcpy(&conn->rx_assembly[conn->rx_len], payload, plen);
            conn->rx_len += plen;
            if (ll_ops.data_rx)
                ll_ops.data_rx(conn, conn->rx_assembly, conn->rx_len);
        }
    }
}

u16 le_ll_event_channel(struct le_ll_conn *conn)
{
    if (!conn)
        return 0;
    return le_csa2((u16)((conn->access_address & 0xFFFF) ^
                         (conn->access_address >> 16)),
                   conn->event_counter, conn->channel_map);
}

void le_ll_tick(u32 slot)
{
    for (unsigned int i = 0; i < LE_LL_MAX_CONN; i++) {
        struct le_ll_conn *conn = &conns[i];
        if (!conn->used)
            continue;

        if (conn->state == LE_LL_DISCONNECTING) {
            conn->used = false;
            if (ll_ops.disconnected)
                ll_ops.disconnected(conn);
            continue;
        }

        u32 timeout_slots = (u32)conn->timeout * 16;
        if (timeout_slots && slot - conn->last_rx_slot > timeout_slots) {
            conn->used = false;
            if (ll_ops.disconnected)
                ll_ops.disconnected(conn);
            continue;
        }

        if (slot < conn->next_event_slot)
            continue;

        radio_tune(le_ll_event_channel(conn));
        if (conn->pending_ack && conn->pending_len)
            radio_tx(conn->pending_tx, conn->pending_len);

        if (conn->pending_ack && conn->pending_len)
            conn->retransmits++;

        conn->event_counter++;
        conn->next_event_slot = slot + (u32)conn->interval * 16 / 10;
    }
}
