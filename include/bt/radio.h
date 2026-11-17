#ifndef __BT_RADIO_H__
#define __BT_RADIO_H__

#include <core/types.h>
#include <driver/regmap.h>

/*
 * Radio front end contract.
 *
 * The baseband owns timing and packet scheduling; the radio only tunes,
 * transmits bytes and reports RSSI.  Register level access is exposed for
 * calibration and power tables.
 */

struct bb_radio {
    const char *name;
    int (*open)(void);
    int (*close)(void);
    int (*set_channel)(u16 channel);
    int (*set_power)(s8 dbm);
    int (*tx)(const u8 *data, u16 len);
    int (*rx)(u8 *data, u16 cap);
    int (*get_rssi)(s8 *dbm);
    int (*calibrate)(void);
};

int radio_register(const struct bb_radio *radio);
const struct bb_radio *radio_get(void);
int radio_open(void);
int radio_close(void);
int radio_tune(u16 channel);
int radio_tx(const u8 *data, u16 len);
int radio_rx(u8 *data, u16 cap);
int radio_power(s8 dbm);
int radio_rssi(s8 *dbm);

/* Optional register view for calibration tables. */
void radio_set_regmap(struct regmap *map);
struct regmap *radio_regmap(void);

#endif /* __BT_RADIO_H__ */
