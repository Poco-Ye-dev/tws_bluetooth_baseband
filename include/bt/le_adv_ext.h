#ifndef __BT_LE_ADV_EXT_H__
#define __BT_LE_ADV_EXT_H__

#include <core/types.h>
#include <bt/types.h>

/*
 * LE extended and periodic advertising.
 *
 * Primary ADV_EXT_IND carries a pointer to an auxiliary PDU on a data
 * channel; periodic advertising adds sync information so a receiver can
 * follow a train of AUX_SYNC_IND packets.
 */

#define LE_EXT_ADV_MAX 255
#define LE_EXT_MAX_SETS 2

struct le_ext_adv_config {
    u8 type;
    u16 interval;
    u8 primary_channel;
    bool periodic;
    u16 periodic_interval;
    u16 sync_handle;
    u8 tx_power;
};

struct le_ext_adv {
    bool used;
    bool enabled;
    struct le_ext_adv_config config;
    u8 data[LE_EXT_ADV_MAX];
    u16 len;
    u8 aux_channel;
    u16 sequence;
    u32 next_slot;
};

void le_ext_adv_init(void);
struct le_ext_adv *le_ext_adv_open(const struct le_ext_adv_config *config);
int le_ext_adv_set_data(struct le_ext_adv *adv, const u8 *data, u16 len);
int le_ext_adv_enable(struct le_ext_adv *adv, bool enable);
void le_ext_adv_tick(u32 slot);

#endif /* __BT_LE_ADV_EXT_H__ */
