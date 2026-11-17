#include <bt/baseband.h>
#include <core/string.h>

/*
 * Hop selection.
 *
 * BR/EDR uses the 79 channel persistent hopping sequence; the adaptive map
 * remaps a masked channel to the next usable one.  LE uses 37 data channels
 * and three advertising channels, selected independently of the access
 * address so a sniffer can follow the event counter.
 */

static u8 afh_map[10];
static bool afh_enabled;

void bb_afh_set(const u8 *map)
{
    if (!map) {
        afh_enabled = false;
        return;
    }
    memcpy(afh_map, map, sizeof(afh_map));
    afh_enabled = true;
}

static bool channel_used(u16 channel)
{
    if (!afh_enabled)
        return true;
    return (afh_map[channel >> 3] >> (channel & 7)) & 1;
}

u16 bb_hop_br(const bt_addr *master, u32 clock)
{
    u32 lap = (u32)master->b[0] | ((u32)master->b[1] << 8) |
              ((u32)master->b[2] << 16);
    u16 channel = (u16)((lap + clock * 7u) % BB_CHANNELS_BR);

    for (u16 i = 0; i < BB_CHANNELS_BR; i++) {
        u16 candidate = (u16)((channel + i) % BB_CHANNELS_BR);
        if (channel_used(candidate))
            return candidate;
    }
    return channel;
}

u16 bb_hop_le(u32 access_address, u16 event_counter, const u8 *channel_map)
{
    u16 channel = (u16)((access_address + (u32)event_counter * 5u) % 37);

    for (u16 i = 0; i < 37; i++) {
        u16 candidate = (u16)((channel + i) % 37);
        if (!channel_map ||
            ((channel_map[candidate >> 3] >> (candidate & 7)) & 1))
            return candidate;
    }
    return channel;
}

u16 bb_le_adv_channel(u8 index)
{
    static const u16 adv[3] = { 37, 38, 39 };
    return adv[index % 3];
}

void bb_afh_reset(void)
{
    memset(afh_map, 0xFF, sizeof(afh_map));
    afh_enabled = false;
}
