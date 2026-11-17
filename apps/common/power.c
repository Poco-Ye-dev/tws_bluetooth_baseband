#include <app/power.h>
#include <core/string.h>

int power_init(struct power_monitor *pm, const struct power_config *config)
{
    if (!pm || !config || !config->adc)
        return -1;

    memset(pm, 0, sizeof(*pm));
    pm->config = *config;
    if (!pm->config.samples)
        pm->config.samples = 8;
    if (!pm->config.low_mv)
        pm->config.low_mv = 3400;
    if (!pm->config.critical_mv)
        pm->config.critical_mv = 3200;
    if (!pm->config.charge_detect_mv)
        pm->config.charge_detect_mv = 4200;

    pm->state = POWER_NORMAL;
    return 0;
}

void power_update(struct power_monitor *pm)
{
    u16 mv;
    enum power_state next;

    if (!pm)
        return;

    if (adc_read_average(pm->config.adc, pm->config.channel,
                         (u8)pm->config.samples, &mv))
        return;

    pm->voltage_mv = mv;

    if (mv >= pm->config.charge_detect_mv)
        next = POWER_CHARGING;
    else if (mv <= pm->config.critical_mv)
        next = POWER_CRITICAL;
    else if (mv <= pm->config.low_mv)
        next = POWER_LOW;
    else
        next = POWER_NORMAL;

    if (next != pm->state) {
        pm->state = next;
        if (pm->callback)
            pm->callback(next, mv, pm->arg);
    }
}

enum power_state power_state(const struct power_monitor *pm)
{
    return pm ? pm->state : POWER_NORMAL;
}

u16 power_voltage(const struct power_monitor *pm)
{
    return pm ? pm->voltage_mv : 0;
}

u8 power_percent(const struct power_monitor *pm)
{
    u16 mv;
    u32 percent;

    if (!pm)
        return 0;

    mv = pm->voltage_mv;
    if (mv >= 4200)
        return 100;
    if (mv <= 3200)
        return 0;

    percent = ((u32)(mv - 3200) * 100) / 1000;
    return (u8)percent;
}
