#ifndef __BT_BASEBAND_H__
#define __BT_BASEBAND_H__

#include <core/types.h>
#include <bt/types.h>

/*
 * Link controller / baseband.
 *
 * Sits below HCI and above the radio.  It owns the slot clock, hop selection,
 * packet framing, ARQ and the LE event scheduler.  All timing is expressed in
 * 625 us slots so the same code drives BR/EDR and LE.
 */

#define BB_SLOT_US        625
#define BB_MAX_LINKS      8
#define BB_MAX_ACL_MTU    1024
#define BB_MAX_RETRIES    3
#define BB_CHANNELS_BR    79
#define BB_CHANNELS_LE    40

enum bb_state {
    BB_STATE_STANDBY = 0,
    BB_STATE_INQUIRY,
    BB_STATE_INQUIRY_SCAN,
    BB_STATE_PAGE,
    BB_STATE_PAGE_SCAN,
    BB_STATE_CONNECTED,
};

enum bb_link_type {
    BB_LINK_ACL = 0,
    BB_LINK_SCO,
};

enum bb_packet_type {
    BB_PKT_ID = 0,
    BB_PKT_NULL,
    BB_PKT_POLL,
    BB_PKT_FHS,
    BB_PKT_DM1,
    BB_PKT_DH1,
    BB_PKT_DM3,
    BB_PKT_DH3,
    BB_PKT_DM5,
    BB_PKT_DH5,
    BB_PKT_HV1,
    BB_PKT_HV2,
    BB_PKT_HV3,
    BB_PKT_DV,
    BB_PKT_EV3,
};

struct bb_config {
    bt_addr local;
    u16 acl_mtu;
    u8 max_retries;
    u8 page_scan;
    u8 inquiry_scan;
    s8 tx_power;
    u8 channel_map[10];
};

struct bb_link {
    bool used;
    u16 handle;
    bt_addr peer;
    enum bb_link_type type;
    u8 seqn;
    u8 arqn;
    bool encrypted;
    u32 last_slot;
    bool sniff_active;
    u16 sniff_interval;
    u16 sniff_attempt;
    u32 sniff_next;
    bool esco_active;
    u16 esco_interval;
};

struct bb_stats {
    u32 slots;
    u32 tx_packets;
    u32 rx_packets;
    u32 retransmits;
    u32 crc_errors;
    u32 hops;
};

struct bb_ops {
    void (*link_up)(struct bb_link *link);
    void (*link_down)(struct bb_link *link, u8 reason);
    void (*acl_rx)(struct bb_link *link, const u8 *data, u16 len);
    void (*sco_rx)(struct bb_link *link, const u8 *data, u16 len);
    void (*inquiry_result)(const bt_addr *addr, s8 rssi);
};

int bb_init(const struct bb_config *config, const struct bb_ops *ops);
int bb_start(void);
int bb_stop(void);
void bb_tick(u32 slot);

int bb_connect(const bt_addr *peer);
int bb_disconnect(u16 handle, u8 reason);
int bb_send_acl(u16 handle, const u8 *data, u16 len);
int bb_send_sco(u16 handle, const u8 *data, u16 len);
int bb_sniff_enable(u16 handle, u16 interval, u16 attempt, u16 timeout);
int bb_sniff_disable(u16 handle);
int bb_esco_add(u16 handle, u16 interval);
int bb_esco_remove(u16 handle);
int bb_lmp_send(u16 handle, u8 opcode, const void *data, u16 len);

int bb_set_scan(bool inquiry, bool page);
int bb_start_inquiry(u32 duration_slots);
int bb_set_tx_power(s8 dbm);

struct bb_link *bb_link_find(u16 handle);
struct bb_link *bb_link_find_addr(const bt_addr *addr);
const struct bb_config *bb_config(void);
enum bb_state bb_state(void);
const struct bb_stats *bb_get_stats(void);
u32 bb_slots(void);

/* Hop selection and adaptive frequency hopping. */
u16 bb_hop_br(const bt_addr *master, u32 clock);
u16 bb_hop_le(u32 access_address, u16 event_counter, const u8 *channel_map);
u16 bb_le_adv_channel(u8 index);
void bb_afh_set(const u8 *map);
void bb_afh_reset(void);

/* Controller integration: register the baseband as an HCI transport. */
int bt_baseband_init(const struct bb_config *config);
void bt_baseband_tick(void);

#endif /* __BT_BASEBAND_H__ */
