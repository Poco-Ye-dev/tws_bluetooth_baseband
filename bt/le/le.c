#include "../common/bb_internal.h"
#include <bt/packet.h>
#include <core/string.h>

/*
 * LE event scheduler.
 *
 * Advertising events rotate over channels 37/38/39; a connection uses the
 * data channel map and its own event counter.  This is independent of the
 * BR/EDR slot engine so dual mode devices can interleave both.
 */

struct le_state {
    const struct bb_config *config;
    const struct bb_ops *ops;
    bool adv_enabled;
    u8 adv_type;
    u16 interval;
    u8 adv_data[31];
    u8 adv_len;
    u8 scan_rsp[31];
    u8 scan_rsp_len;
    u8 adv_index;
    u32 next_adv_slot;
    bool connecting;
    bt_addr peer;
    u8 channel_map[5];
    u32 access_address;
    u16 event_counter;
    u32 next_event_slot;
};

static struct le_state le;

static u16 adv_interval_slots(void)
{
    return le.interval ? le.interval : 0x00A0;
}

void bb_le_init(const struct bb_config *config, const struct bb_ops *ops)
{
    memset(&le, 0, sizeof(le));
    le.config = config;
    le.ops = ops;
    le.interval = 0x00A0;
    memset(le.channel_map, 0xFF, sizeof(le.channel_map));
    le.access_address = 0x8E89BED6u;
}

void bb_le_set_adv_params(u16 interval_min, u16 interval_max, u8 type)
{
    le.interval = interval_min ? interval_min : interval_max;
    if (!le.interval)
        le.interval = 0x00A0;
    le.adv_type = type;
}

void bb_le_set_adv_data(const u8 *data, u8 len)
{
    if (!data || len > sizeof(le.adv_data))
        return;
    memcpy(le.adv_data, data, len);
    le.adv_len = len;
}

void bb_le_set_scan_rsp(const u8 *data, u8 len)
{
    if (!data || len > sizeof(le.scan_rsp))
        return;
    memcpy(le.scan_rsp, data, len);
    le.scan_rsp_len = len;
}

int bb_le_adv_enable(bool enable)
{
    le.adv_enabled = enable;
    le.adv_index = 0;
    le.next_adv_slot = 0;
    return 0;
}

void bb_le_start_connect(const bt_addr *peer)
{
    if (!peer)
        return;
    le.connecting = true;
    le.peer = *peer;
    le.event_counter = 0;
    le.next_event_slot = 0;
}

void bb_le_stop_connect(void)
{
    le.connecting = false;
}

static void send_adv(void)
{
    u8 packet[64];
    u8 channel = (u8)bb_le_adv_channel(le.adv_index++);
    u16 slot = (u16)(le.next_adv_slot / adv_interval_slots());
    u8 pdu_len = (u8)(6 + le.adv_len);

    if (pdu_len > 37)
        pdu_len = 37;

    packet[0] = 0xAA;
    packet[1] = 0xD6;
    packet[2] = 0xBE;
    packet[3] = 0x89;
    packet[4] = 0x8E;
    packet[5] = le.adv_type == 0 ? 0x00 : 0x02;
    packet[6] = pdu_len;

    const bt_addr *local = bb_local_addr();
    for (int i = 0; i < 6; i++)
        packet[7 + i] = local->b[i];
    memcpy(&packet[13], le.adv_data, le.adv_len);

    u16 len = (u16)(7 + pdu_len);
    u32 crc = bb_crc24(&packet[5], (u32)(pdu_len + 2), 0x555555, NULL);
    packet[len++] = (u8)(crc & 0xFF);
    packet[len++] = (u8)((crc >> 8) & 0xFF);
    packet[len++] = (u8)((crc >> 16) & 0xFF);

    radio_tune(channel);
    if (radio_tx(packet, len) == 0) {
        struct bb_stats *stats = bb_stats_mut();
        stats->tx_packets++;
        stats->hops++;
    }
    (void)slot;
}

static void poll_connection(void)
{
    u8 rx[64];
    u16 channel = bb_hop_le(le.access_address, le.event_counter, le.channel_map);

    radio_tune(channel);
    int len = radio_rx(rx, sizeof(rx));

    if (len > 6) {
        u8 pdu_type = (u8)(rx[5] & 0x0F);
        if (pdu_type == 0x05 && le.connecting) {
            struct bb_link *link = bb_link_alloc(&le.peer, BB_LINK_ACL);
            if (link && le.ops && le.ops->link_up)
                le.ops->link_up(link);
            le.connecting = false;
        }
    }

    le.event_counter++;
    struct bb_stats *stats = bb_stats_mut();
    stats->rx_packets++;
}

void bb_le_tick(u32 slot)
{
    if (le.adv_enabled && slot >= le.next_adv_slot) {
        send_adv();
        le.next_adv_slot = slot + adv_interval_slots();
    }

    if (le.connecting && slot >= le.next_event_slot) {
        poll_connection();
        le.next_event_slot = slot + adv_interval_slots();
    }
}
