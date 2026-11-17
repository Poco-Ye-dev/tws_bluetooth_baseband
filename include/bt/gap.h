#ifndef __BT_GAP_H__
#define __BT_GAP_H__

#include <core/types.h>
#include <bt/types.h>

/*
 * Generic access profile: advertising, scanning and connection management.
 *
 * Advertising data is assembled with a small builder that enforces the 31
 * byte limit, so callers cannot emit a malformed advertisement.
 */

#define GAP_ADV_DATA_MAX 31
#define GAP_MAX_FILTERS  8

enum gap_adv_type {
    GAP_ADV_IND = 0,
    GAP_ADV_DIRECT_IND,
    GAP_ADV_SCAN_IND,
    GAP_ADV_NONCONN_IND,
};

struct gap_adv_config {
    enum gap_adv_type type;
    u16 interval_min;
    u16 interval_max;
    bt_addr direct_peer;
    u8 channel_map;
    u8 filter_policy;
};

struct gap_scan_config {
    bool active;
    u16 interval;
    u16 window;
    u8 filter_policy;
};

struct gap_callbacks {
    void (*connected)(const bt_addr *peer, enum bt_link_type type, u16 handle);
    void (*disconnected)(const bt_addr *peer, u8 reason);
    void (*adv_report)(const bt_addr *peer, s8 rssi, const u8 *data, u8 len);
};

struct adv_builder {
    u8 data[GAP_ADV_DATA_MAX];
    u8 len;
    bool overflow;
};

void gap_init(const struct gap_callbacks *cb);

void adv_builder_reset(struct adv_builder *b);
int adv_builder_add(struct adv_builder *b, u8 type, const void *data, u8 len);
int adv_builder_add_flags(struct adv_builder *b, u8 flags);
int adv_builder_add_name(struct adv_builder *b, const char *name);
int adv_builder_add_uuid16(struct adv_builder *b, u16 uuid);
int adv_builder_add_tx_power(struct adv_builder *b, s8 dbm);

int gap_set_adv_config(const struct gap_adv_config *config);
int gap_set_adv_data(const struct adv_builder *b);
int gap_set_scan_rsp(const struct adv_builder *b);
int gap_start_adv(void);
int gap_stop_adv(void);
int gap_start_scan(const struct gap_scan_config *config);
int gap_stop_scan(void);
int gap_connect(const bt_addr *peer);
int gap_disconnect(u16 handle, u8 reason);
bool gap_adv_active(void);
bool gap_scan_active(void);

#endif /* __BT_GAP_H__ */

void gap_register_handlers(void);
