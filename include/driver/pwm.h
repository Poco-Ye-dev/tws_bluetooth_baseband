#ifndef __DRIVER_PWM_H__
#define __DRIVER_PWM_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * PWM channels drive backlight, motor and buzzer outputs.  Period and duty
 * are expressed in nanoseconds so board files do not bake in a clock rate.
 */

struct pwm_channel {
    u8 id;
    u32 period_ns;
    u32 duty_ns;
    bool inverted;
    bool enabled;
};

struct pwm_controller {
    const char *name;
    unsigned int nchannels;
    int (*configure)(struct pwm_controller *ctrl, struct pwm_channel *ch);
    int (*enable)(struct pwm_controller *ctrl, u8 id, bool enable);
    void *priv;
};

int pwm_register(struct pwm_controller *ctrl);
struct pwm_controller *pwm_get(const char *name);

int pwm_config(struct pwm_controller *ctrl, u8 id, u32 period_ns, u32 duty_ns);
int pwm_enable(struct pwm_controller *ctrl, u8 id, bool enable);
int pwm_set_duty(struct pwm_controller *ctrl, u8 id, u32 duty_ns);
int pwm_set_percent(struct pwm_controller *ctrl, u8 id, u32 period_ns,
                    u8 percent);

#endif /* __DRIVER_PWM_H__ */
