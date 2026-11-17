#ifndef __BT_BB_INTERNAL_H__
#define __BT_BB_INTERNAL_H__

#include <bt/baseband.h>
#include <bt/radio.h>

/* Shared between the baseband modules; not part of the public API. */

struct bb_link *bb_link_alloc(const bt_addr *peer, enum bb_link_type type);
void bb_link_free(struct bb_link *link);
const struct bb_ops *bb_ops(void);
const bt_addr *bb_local_addr(void);
s8 bb_tx_power(void);
struct bb_stats *bb_stats_mut(void);

/* LE scheduler. */
void bb_le_init(const struct bb_config *config, const struct bb_ops *ops);
void bb_le_tick(u32 slot);
void bb_le_set_adv_params(u16 interval_min, u16 interval_max, u8 type);
void bb_le_set_adv_data(const u8 *data, u8 len);
void bb_le_set_scan_rsp(const u8 *data, u8 len);
int bb_le_adv_enable(bool enable);
void bb_le_start_connect(const bt_addr *peer);
void bb_le_stop_connect(void);

/* SCO / eSCO scheduler. */
void bb_sco_init(void);
void bb_sco_tick(u32 slot);
int bb_sco_send(struct bb_link *link, const u8 *data, u16 len);

#endif /* __BT_BB_INTERNAL_H__ */
