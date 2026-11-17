#include <bt/le_csa.h>

static bool used(const u8 map[5], u16 channel)
{
    return (map[channel >> 3] >> (channel & 7)) & 1;
}

u8 le_channel_count(const u8 map[5])
{
    u8 count = 0;
    for (u16 ch = 0; ch < 37; ch++) {
        if (used(map, ch))
            count++;
    }
    return count;
}

static u16 permute(u16 v)
{
    v = (u16)(((v & 0xAAAA) >> 1) | ((v & 0x5555) << 1));
    v = (u16)(((v & 0xCCCC) >> 2) | ((v & 0x3333) << 2));
    v = (u16)(((v & 0xF0F0) >> 4) | ((v & 0x0F0F) << 4));
    v = (u16)(((v & 0xFF00) >> 8) | ((v & 0x00FF) << 8));
    return (u16)(v ^ (v >> 4));
}

u16 le_csa2(u16 channel_id, u16 event_counter, const u8 map[5])
{
    u16 prn = (u16)(permute(channel_id) ^ event_counter);
    u16 unmapped = (u16)(prn % 37);
    u8 num_used = le_channel_count(map);
    u8 remap;

    if (!num_used)
        return 0;
    if (used(map, unmapped))
        return unmapped;

    remap = (u8)(prn % num_used);
    for (u16 ch = 0; ch < 37; ch++) {
        if (!used(map, ch))
            continue;
        if (remap == 0)
            return ch;
        remap--;
    }
    return 0;
}

u16 le_csa1(u16 channel_id, u16 event_counter, const u8 map[5])
{
    u16 hop = (u16)(((channel_id * 5) + (event_counter * 7)) % 37);

    for (u16 i = 0; i < 37; i++) {
        u16 ch = (u16)((hop + i) % 37);
        if (used(map, ch))
            return ch;
    }
    return hop;
}
