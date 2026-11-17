#ifndef __BT_LE_CSA_H__
#define __BT_LE_CSA_H__

#include <core/types.h>

/*
 * LE channel selection algorithms.
 *
 * CSA#1 (legacy) is kept for older peers; CSA#2 is the default and uses the
 * permute function plus the event counter so a sniffer can predict the next
 * channel from the access address alone.
 */

u16 le_csa1(u16 channel_id, u16 event_counter, const u8 map[5]);
u16 le_csa2(u16 channel_id, u16 event_counter, const u8 map[5]);
u8 le_channel_count(const u8 map[5]);

#endif /* __BT_LE_CSA_H__ */
