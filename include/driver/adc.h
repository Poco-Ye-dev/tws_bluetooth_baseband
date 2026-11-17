#ifndef __DRIVER_ADC_H__
#define __DRIVER_ADC_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Generic ADC channel interface.  Battery, NTC and key matrices all read
 * through here so calibration lives in one place.
 */

struct adc_channel {
    const char *name;
    u8 id;
    u16 reference_mv;
    u8 resolution;
};

struct adc_controller {
    const char *name;
    unsigned int nchannels;
    u16 reference_mv;
    u8 resolution;
    int (*read_raw)(struct adc_controller *ctrl, u8 channel, u16 *value);
    int (*read_mv)(struct adc_controller *ctrl, u8 channel, u16 *mv);
    int (*calibrate)(struct adc_controller *ctrl);
    void *priv;
};

int adc_register(struct adc_controller *ctrl);
struct adc_controller *adc_get(const char *name);

int adc_read_raw(struct adc_controller *ctrl, u8 channel, u16 *value);
int adc_read_mv(struct adc_controller *ctrl, u8 channel, u16 *mv);
int adc_read_average(struct adc_controller *ctrl, u8 channel, u8 samples,
                     u16 *mv);
int adc_calibrate(struct adc_controller *ctrl);

#endif /* __DRIVER_ADC_H__ */
