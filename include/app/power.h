#ifndef __APP_POWER_H__
#define __APP_POWER_H__

#include <core/types.h>
#include <driver/adc.h>

/*
 * Battery and charger supervision.
 *
 * Voltage is filtered by the ADC layer, then compared against hysteresis
 * thresholds to avoid state flapping around a single boundary.
 */

enum power_state {
    POWER_NORMAL = 0,
    POWER_LOW,
    POWER_CRITICAL,
    POWER_CHARGING,
};

struct power_config {
    struct adc_controller *adc;
    u8 channel;
    u16 low_mv;
    u16 critical_mv;
    u16 charge_detect_mv;
    u16 samples;
};

struct power_monitor {
    struct power_config config;
    enum power_state state;
    u16 voltage_mv;
    void (*callback)(enum power_state state, u16 mv, void *arg);
    void *arg;
};

int power_init(struct power_monitor *pm, const struct power_config *config);
void power_update(struct power_monitor *pm);
enum power_state power_state(const struct power_monitor *pm);
u16 power_voltage(const struct power_monitor *pm);
u8 power_percent(const struct power_monitor *pm);

#endif /* __APP_POWER_H__ */
