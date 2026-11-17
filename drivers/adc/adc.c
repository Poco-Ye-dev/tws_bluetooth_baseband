#include <driver/adc.h>
#include <core/string.h>
#include <core/math.h>

/*
 * ADC core with oversampling and a median filter.
 *
 * Median rejection removes spike noise (motor commutation, RF bursts) before
 * averaging, which keeps the battery gauge from jumping on a single sample.
 */

#define ADC_FILTER_MAX 16

static struct adc_controller *controllers[2];
static unsigned int controller_count;

int adc_register(struct adc_controller *ctrl)
{
    if (!ctrl || !ctrl->read_raw || controller_count >= 2)
        return -EINVAL;

    controllers[controller_count++] = ctrl;
    return 0;
}

struct adc_controller *adc_get(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

int adc_read_raw(struct adc_controller *ctrl, u8 channel, u16 *value)
{
    if (!ctrl || !ctrl->read_raw || channel >= ctrl->nchannels)
        return -EINVAL;
    return ctrl->read_raw(ctrl, channel, value);
}

int adc_read_mv(struct adc_controller *ctrl, u8 channel, u16 *mv)
{
    u16 raw;
    int ret;

    if (!ctrl || !mv || channel >= ctrl->nchannels)
        return -EINVAL;

    if (ctrl->read_mv)
        return ctrl->read_mv(ctrl, channel, mv);

    ret = ctrl->read_raw(ctrl, channel, &raw);
    if (ret)
        return ret;

    u32 full = (1u << ctrl->resolution) - 1;
    *mv = (u16)(((u32)raw * ctrl->reference_mv) / full);
    return 0;
}

static void sort(u16 *values, u8 count)
{
    for (u8 i = 1; i < count; i++) {
        u16 key = values[i];
        int j = i - 1;
        while (j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = key;
    }
}

int adc_read_average(struct adc_controller *ctrl, u8 channel, u8 samples,
                     u16 *mv)
{
    u16 values[ADC_FILTER_MAX];
    u32 sum = 0;

    if (samples == 0)
        return -EINVAL;
    if (samples > ADC_FILTER_MAX)
        samples = ADC_FILTER_MAX;

    for (u8 i = 0; i < samples; i++) {
        int ret = adc_read_mv(ctrl, channel, &values[i]);
        if (ret)
            return ret;
    }

    sort(values, samples);

    /* Drop the lowest and highest quarter before averaging. */
    u8 trim = samples >= 4 ? (u8)(samples / 4) : 0;
    for (u8 i = trim; i < samples - trim; i++)
        sum += values[i];

    *mv = (u16)(sum / (samples - 2 * trim));
    return 0;
}

int adc_calibrate(struct adc_controller *ctrl)
{
    if (!ctrl)
        return -EINVAL;

    if (!ctrl->calibrate) {
        /* Zero scale by measuring the internal reference channel if present. */
        u16 mv;
        int ret = adc_read_mv(ctrl, 0, &mv);
        if (ret)
            return ret;
        if (mv == 0)
            return -ERANGE;
        return 0;
    }
    return ctrl->calibrate(ctrl);
}
