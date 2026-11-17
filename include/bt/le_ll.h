#ifndef __BT_LE_LL_H__
#define __BT_LE_LL_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/radio.h>
#include <crypto/aes.h>

/*
 * LE Link Layer.
 *
 * Implements the data channel PDU format, SN/NESN retransmission, the LL
 * control procedure set, connection event timing and AES-CCM encryption.
 * The baseband supplies the slot clock and the radio; this module owns the
 * protocol state for one connection.
 */

#define LE_LL_MAX_PDU 255
#define LE_LL_MAX_CONN 4
#define LE_LL_FEATURE_LEN 8

enum le_llid {
    LE_LLID_DATA_CONT = 0x01,
    LE_LLID_DATA_START = 0x02,
    LE_LLID_CONTROL = 0x03,
};

enum le_ll_opcode {
    LE_LL_CONNECTION_UPDATE_IND = 0x00,
    LE_LL_CHANNEL_MAP_IND        = 0x01,
    LE_LL_TERMINATE_IND          = 0x02,
    LE_LL_ENC_REQ                = 0x03,
    LE_LL_ENC_RSP                = 0x04,
    LE_LL_START_ENC_REQ          = 0x05,
    LE_LL_START_ENC_RSP          = 0x06,
    LE_LL_UNKNOWN_RSP            = 0x07,
    LE_LL_FEATURE_REQ            = 0x08,
    LE_LL_FEATURE_RSP            = 0x09,
    LE_LL_PAUSE_ENC_REQ          = 0x0A,
    LE_LL_PAUSE_ENC_RSP          = 0x0B,
    LE_LL_VERSION_IND            = 0x0C,
    LE_LL_REJECT_IND             = 0x0D,
    LE_LL_CONNECTION_PARAM_REQ   = 0x0E,
    LE_LL_CONNECTION_PARAM_RSP   = 0x0F,
    LE_LL_REJECT_EXT_IND         = 0x10,
    LE_LL_PING_REQ               = 0x11,
    LE_LL_PING_RSP               = 0x12,
    LE_LL_LENGTH_REQ             = 0x13,
    LE_LL_LENGTH_RSP             = 0x14,
    LE_LL_PHY_REQ                = 0x15,
    LE_LL_PHY_RSP                = 0x16,
    LE_LL_PHY_UPDATE_IND         = 0x17,
    LE_LL_MIN_USED_CHANNELS_IND  = 0x18,
};

enum le_ll_state {
    LE_LL_IDLE = 0,
    LE_LL_CONNECTED,
    LE_LL_ENCRYPTING,
    LE_LL_ENCRYPTED,
    LE_LL_DISCONNECTING,
};

struct le_ll_conn {
    bool used;
    u16 handle;
    bt_addr peer;
    u32 access_address;
    u32 crc_init;
    u8 channel_map[5];
    u16 event_counter;
    u16 interval;                   /* 1.25 ms units */
    u16 latency;
    u16 timeout;                    /* 10 ms units */
    u32 next_event_slot;
    u32 last_rx_slot;
    u8 sn;
    u8 nesn;
    u8 tx_seq;
    u8 features[LE_LL_FEATURE_LEN];
    enum le_ll_state state;
    bool encrypted;
    bool pending_ack;
    u8 ltk[16];
    u8 sk[16];
    u8 iv[8];
    u32 packet_counter;
    u8 pending_tx[LE_LL_MAX_PDU];
    u16 pending_len;
    u8 pending_llid;
    u8 rx_assembly[LE_LL_MAX_PDU];
    u16 rx_len;
    u8 rssi;
    u32 packets_rx;
    u32 packets_tx;
    u32 retransmits;
};

typedef void (*le_ll_rx_cb_t)(struct le_ll_conn *conn, const u8 *data, u16 len);
typedef void (*le_ll_event_cb_t)(struct le_ll_conn *conn);

struct le_ll_ops {
    le_ll_rx_cb_t data_rx;
    le_ll_event_cb_t connected;
    le_ll_event_cb_t disconnected;
    le_ll_event_cb_t encrypted_changed;
};

void le_ll_init(const struct le_ll_ops *ops);
struct le_ll_conn *le_ll_create(u16 handle, const bt_addr *peer,
                                u32 access_address, u32 crc_init,
                                u16 interval, u16 latency, u16 timeout);
struct le_ll_conn *le_ll_find(u16 handle);
int le_ll_send(struct le_ll_conn *conn, const u8 *data, u16 len);
int le_ll_send_control(struct le_ll_conn *conn, u8 opcode,
                       const u8 *payload, u8 len);
int le_ll_update_params(struct le_ll_conn *conn, u16 interval, u16 latency,
                        u16 timeout);
int le_ll_update_channel_map(struct le_ll_conn *conn, const u8 map[5]);
int le_ll_start_encryption(struct le_ll_conn *conn,
                           const u8 rand[8], const u8 ediv[2],
                           const u8 ltk[16]);
int le_ll_terminate(struct le_ll_conn *conn, u8 reason);
void le_ll_tick(u32 slot);
void le_ll_rx(struct le_ll_conn *conn, const u8 *pdu, u16 len);
u16 le_ll_event_channel(struct le_ll_conn *conn);

#endif /* __BT_LE_LL_H__ */
