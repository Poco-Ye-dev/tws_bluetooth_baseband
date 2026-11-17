#ifndef __BT_LE_ISO_H__
#define __BT_LE_ISO_H__

#include <core/types.h>

/*
 * Isochronous channels and the ISO adaptation layer.
 *
 * CIS carries bidirectional audio for LE Audio; BIS is connectionless
 * broadcast.  ISOAL turns an SDU stream into link layer PDUs, either
 * unframed (one SDU per PDU) or framed (segmented with sequence and time
 * offset).
 */

#define LE_ISO_MAX_SDU 251
#define LE_ISO_MAX_CHANNELS 4

enum le_iso_type {
    LE_ISO_CIS = 0,
    LE_ISO_BIS,
};

enum le_iso_framing {
    LE_ISO_UNFRAMED = 0,
    LE_ISO_FRAMED,
};

struct le_iso_config {
    enum le_iso_type type;
    enum le_iso_framing framing;
    u16 sdu_len;
    u16 pdu_len;
    u32 interval_us;
    u16 latency_ms;
    u8 channels;
};

struct le_iso_stats {
    u32 sdu_sent;
    u32 sdu_received;
    u32 lost;
    u32 late;
    u32 reassembled;
};

struct le_iso {
    bool used;
    bool running;
    u16 handle;
    struct le_iso_config config;
    u32 next_event_slot;
    u16 sequence;
    u8 tx_buffer[LE_ISO_MAX_SDU];
    u16 tx_len;
    u8 rx_assembly[LE_ISO_MAX_SDU];
    u16 rx_len;
    u8 rx_seq;
    struct le_iso_stats stats;
};

void le_iso_init(void);
struct le_iso *le_iso_open(u16 handle, const struct le_iso_config *config);
int le_iso_start(struct le_iso *iso);
int le_iso_stop(struct le_iso *iso);
int le_iso_send(struct le_iso *iso, const u8 *sdu, u16 len);
int le_iso_receive(struct le_iso *iso, const u8 *pdu, u16 len);
void le_iso_tick(u32 slot);
const struct le_iso_stats *le_iso_stats(struct le_iso *iso);

#endif /* __BT_LE_ISO_H__ */
