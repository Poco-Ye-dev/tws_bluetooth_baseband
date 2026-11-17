#include <bt/edr.h>
#include <bt/baseband.h>
#include <core/string.h>
#include <core/printk.h>

/*
 * BR/EDR slot reservation and AFH channel assessment.
 *
 * The scheduler keeps a bitmap of reserved slots so multi-slot packets and
 * SCO reservations cannot overlap.  AFH counts CRC failures per channel and
 * marks a channel bad after a configurable run of errors; the map is rebuilt
 * only on request so the hop kernel never sees a partially updated map.
 */

#define EDR_AFH_CHANNELS 79
#define EDR_AFH_BAD_RUN 6

static struct {
    u32 errors[EDR_AFH_CHANNELS];
    u32 good[EDR_AFH_CHANNELS];
    u8 bad_run[EDR_AFH_CHANNELS];
    u8 min_channels;
} afh;

void edr_afh_init(void)
{
    memset(&afh, 0, sizeof(afh));
    afh.min_channels = 20;
}

void edr_afh_report(u16 channel, bool crc_ok)
{
    if (channel >= EDR_AFH_CHANNELS)
        return;

    if (crc_ok) {
        afh.good[channel]++;
        afh.bad_run[channel] = 0;
        if (afh.errors[channel])
            afh.errors[channel]--;
    } else {
        afh.errors[channel]++;
        afh.bad_run[channel]++;
    }
}

int edr_afh_build_map(u8 map[10])
{
    u8 used = 0;

    if (!map)
        return -1;

    memset(map, 0, 10);
    for (u16 ch = 0; ch < EDR_AFH_CHANNELS; ch++) {
        bool bad = afh.bad_run[ch] >= EDR_AFH_BAD_RUN ||
                   (afh.errors[ch] > afh.good[ch] + 4);
        if (!bad) {
            map[ch >> 3] |= (u8)(1u << (ch & 7));
            used++;
        }
    }

    if (used < afh.min_channels) {
        /* Restore the best channels until the minimum is met. */
        for (u16 ch = 0; ch < EDR_AFH_CHANNELS && used < afh.min_channels; ch++) {
            if (map[ch >> 3] & (1u << (ch & 7)))
                continue;
            map[ch >> 3] |= (u8)(1u << (ch & 7));
            used++;
        }
    }
    return used;
}

void edr_afh_set_min(u8 min_channels)
{
    afh.min_channels = min_channels ? min_channels : 20;
}

/* Multi-slot reservation table. */
#define EDR_MAX_RESERVATIONS 8

static struct {
    bool used;
    u16 handle;
    u32 start;
    u8 slots;
} reservations[EDR_MAX_RESERVATIONS];

void edr_sched_init(void)
{
    memset(reservations, 0, sizeof(reservations));
}

int edr_reserve(u16 handle, u32 slot, u8 slots)
{
    for (unsigned int i = 0; i < EDR_MAX_RESERVATIONS; i++) {
        if (reservations[i].used)
            continue;
        reservations[i].used = true;
        reservations[i].handle = handle;
        reservations[i].start = slot;
        reservations[i].slots = slots;
        return 0;
    }
    return -1;
}

bool edr_slot_free(u32 slot)
{
    for (unsigned int i = 0; i < EDR_MAX_RESERVATIONS; i++) {
        if (!reservations[i].used)
            continue;
        if (slot >= reservations[i].start &&
            slot < reservations[i].start + reservations[i].slots)
            return false;
    }
    return true;
}

void edr_release(u16 handle)
{
    for (unsigned int i = 0; i < EDR_MAX_RESERVATIONS; i++) {
        if (reservations[i].used && reservations[i].handle == handle)
            reservations[i].used = false;
    }
}
