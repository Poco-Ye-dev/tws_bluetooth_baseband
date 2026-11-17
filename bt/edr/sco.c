#include "../common/bb_internal.h"
#include <bt/packet.h>
#include <core/string.h>

/*
 * SCO / eSCO scheduler.
 *
 * SCO reserves a fixed slot pair every N slots and always uses HV3 frames of
 * 30 bytes, which is enough for 8 kHz narrow band voice at 64 kbit/s.
 */

#define SCO_MAX 4
#define SCO_HV3_BYTES 30

struct sco_link {
    bool used;
    u16 handle;
    u16 interval;
    u32 next_slot;
    u8 pending[SCO_HV3_BYTES];
    u16 pending_len;
};

static struct sco_link sco[SCO_MAX];

void bb_sco_init(void)
{
    memset(sco, 0, sizeof(sco));
}

static struct sco_link *sco_for(u16 handle, bool create)
{
    for (unsigned int i = 0; i < SCO_MAX; i++) {
        if (sco[i].used && sco[i].handle == handle)
            return &sco[i];
    }
    if (!create)
        return NULL;

    for (unsigned int i = 0; i < SCO_MAX; i++) {
        if (!sco[i].used) {
            memset(&sco[i], 0, sizeof(sco[i]));
            sco[i].used = true;
            sco[i].handle = handle;
            sco[i].interval = 6;
            return &sco[i];
        }
    }
    return NULL;
}

int bb_sco_send(struct bb_link *link, const u8 *data, u16 len)
{
    struct sco_link *slot;

    if (!link || !data)
        return -1;

    slot = sco_for(link->handle, true);
    if (!slot)
        return -2;

    if (len > SCO_HV3_BYTES)
        len = SCO_HV3_BYTES;
    memcpy(slot->pending, data, len);
    slot->pending_len = len;
    return 0;
}

void bb_sco_tick(u32 slot_index)
{
    for (unsigned int i = 0; i < SCO_MAX; i++) {
        struct sco_link *link = &sco[i];
        struct bb_link *l;

        if (!link->used)
            continue;
        if (slot_index < link->next_slot)
            continue;

        l = bb_link_find(link->handle);
        if (!l) {
            link->used = false;
            continue;
        }

        u8 frame[4 + SCO_HV3_BYTES + 4];
        struct bb_header hdr = {
            .lt_addr = (u8)(link->handle & 0x07),
            .type = BB_PKT_HV3,
            .flow = 1,
            .arqn = 1,
            .seqn = 0,
        };
        u8 header[4];
        u32 bits = 0;
        u8 channel = (u8)((slot_index / link->interval) % BB_CHANNELS_BR);

        bb_header_encode(&hdr, header, &bits);
        frame[0] = header[0];
        frame[1] = header[1];
        frame[2] = header[2];
        memcpy(&frame[3], link->pending, SCO_HV3_BYTES);

        bb_whiten(&frame[3], SCO_HV3_BYTES, (u8)(slot_index & 0x3F), channel);

        u32 crc = bb_crc24(link->pending, SCO_HV3_BYTES, 0, &l->peer);
        frame[3 + SCO_HV3_BYTES] = (u8)(crc & 0xFF);
        frame[4 + SCO_HV3_BYTES] = (u8)((crc >> 8) & 0xFF);
        frame[5 + SCO_HV3_BYTES] = (u8)((crc >> 16) & 0xFF);

        radio_tune(channel);
        radio_tx(frame, (u16)(6 + SCO_HV3_BYTES));

        struct bb_stats *stats = bb_stats_mut();
        stats->tx_packets++;

        link->next_slot += link->interval;
    }
}
