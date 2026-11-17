#include <system/clock.h>
#include <core/string.h>

/*
 * tcl family clock tree.  Rates are limited to the PLL/mux combinations the
 * part supports; the port rounds down and reports the achieved value.
 */

struct clock_desc {
    u32 rate;
    bool enabled;
};

static const u32 cpu_rates[] = { 24000000, 48000000, 96000000, 120000000, 160000000 };
static const u32 audio_rates[] = { 12288000, 24576000, 45158400, 49152000 };
static const u32 bus_rates[] = { 12000000, 24000000, 48000000, 60000000 };
static const u32 flash_rates[] = { 24000000, 48000000, 80000000 };

static struct clock_desc clocks[CLOCK_COUNT];

static const u32 *rate_table(enum clock_id id, u32 *count)
{
    switch (id) {
    case CLOCK_CPU:
        *count = sizeof(cpu_rates) / sizeof(cpu_rates[0]);
        return cpu_rates;
    case CLOCK_AUDIO:
        *count = sizeof(audio_rates) / sizeof(audio_rates[0]);
        return audio_rates;
    case CLOCK_FLASH:
        *count = sizeof(flash_rates) / sizeof(flash_rates[0]);
        return flash_rates;
    default:
        *count = sizeof(bus_rates) / sizeof(bus_rates[0]);
        return bus_rates;
    }
}

int clock_init(void)
{
    clocks[CLOCK_CPU].rate = 120000000;
    clocks[CLOCK_BUS].rate = 48000000;
    clocks[CLOCK_AUDIO].rate = 24576000;
    clocks[CLOCK_FLASH].rate = 48000000;
    for (int i = 0; i < CLOCK_COUNT; i++)
        clocks[i].enabled = true;
    return 0;
}

u32 clock_get(enum clock_id id)
{
    return id < CLOCK_COUNT ? clocks[id].rate : 0;
}

u32 clock_set(enum clock_id id, u32 hz)
{
    u32 count;
    const u32 *table;
    u32 best = 0;

    if (id >= CLOCK_COUNT)
        return 0;

    table = rate_table(id, &count);
    for (u32 i = 0; i < count; i++) {
        if (table[i] <= hz)
            best = table[i];
    }
    if (!best)
        best = table[0];

    clocks[id].rate = best;
    return best;
}

int clock_enable(enum clock_id id, bool enable)
{
    if (id >= CLOCK_COUNT)
        return -EINVAL;
    clocks[id].enabled = enable;
    return 0;
}

bool clock_enabled(enum clock_id id)
{
    return id < CLOCK_COUNT && clocks[id].enabled;
}
