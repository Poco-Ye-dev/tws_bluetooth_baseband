#ifndef __BT_EDR_H__
#define __BT_EDR_H__

#include <core/types.h>

/*
 * BR/EDR packet and link management definitions.
 *
 * Packet choice depends on payload size and modulation: basic rate uses GFSK,
 * EDR uses pi/4-DQPSK (2-DHx) or 8DPSK (3-DHx).  The scheduler caps a
 * transmission by the number of slots it occupies so SCO reservations are
 * never overrun.
 */

enum edr_packet_type {
    EDR_PKT_NULL = 0,
    EDR_PKT_POLL,
    EDR_PKT_FHS,
    EDR_PKT_DM1,
    EDR_PKT_DH1,
    EDR_PKT_DM3,
    EDR_PKT_DH3,
    EDR_PKT_DM5,
    EDR_PKT_DH5,
    EDR_PKT_HV1,
    EDR_PKT_HV2,
    EDR_PKT_HV3,
    EDR_PKT_DV,
    EDR_PKT_EV3,
    EDR_PKT_EV4,
    EDR_PKT_EV5,
    EDR_PKT_2DH1,
    EDR_PKT_2DH3,
    EDR_PKT_2DH5,
    EDR_PKT_3DH1,
    EDR_PKT_3DH3,
    EDR_PKT_3DH5,
};

enum edr_modulation {
    EDR_MOD_GFSK = 0,
    EDR_MOD_DQPSK,
    EDR_MOD_8DPSK,
};

enum edr_fec {
    EDR_FEC_NONE = 0,
    EDR_FEC_1_3,
    EDR_FEC_2_3,
};

struct edr_pkt_info {
    const char *name;
    enum edr_packet_type type;
    u8 slots;
    enum edr_fec fec;
    enum edr_modulation modulation;
    u16 max_payload;
};

const struct edr_pkt_info *edr_pkt_info(enum edr_packet_type type);
enum edr_packet_type edr_pkt_select(u16 payload, enum edr_modulation mod,
                                    bool voice);
u32 edr_fec23_encode(const u8 *in, u32 bytes, u8 *out);
u32 edr_fec23_decode(const u8 *in, u32 bytes, u8 *out);

struct edr_esco_params {
    u16 te;
    u16 timeout;                         /* reserved slots after the window */
    u16 we;                         /* retransmission window */
    u16 interval;
    u16 retries;
    enum edr_packet_type packet;
};

int edr_esco_negotiate(const struct edr_esco_params *current,
                       const struct edr_esco_params *requested,
                       struct edr_esco_params *negotiated);

struct edr_sniff {
    bool active;
    u16 attempt;
    u16 timeout;
    u16 interval;
    u32 next_slot;
    u32 anchor_slot;
};

void edr_sniff_start(struct edr_sniff *sniff, u16 interval, u16 attempt,
                     u16 timeout, u32 now);
void edr_sniff_stop(struct edr_sniff *sniff);
bool edr_sniff_tick(struct edr_sniff *sniff, u32 slot);

s8 edr_power_step(s8 rssi, s8 target, s8 *current);

void edr_afh_init(void);
void edr_afh_report(u16 channel, bool crc_ok);
int edr_afh_build_map(u8 map[10]);
void edr_afh_set_min(u8 min_channels);
void edr_sched_init(void);
int edr_reserve(u16 handle, u32 slot, u8 slots);
bool edr_slot_free(u32 slot);
void edr_release(u16 handle);

#endif /* __BT_EDR_H__ */
