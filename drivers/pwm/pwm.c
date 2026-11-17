#include <driver/pwm.h>
#include <core/string.h>

#define PWM_MAX_CONTROLLERS 2

static struct pwm_controller *controllers[PWM_MAX_CONTROLLERS];
static unsigned int controller_count;

int pwm_register(struct pwm_controller *ctrl)
{
    if (!ctrl || !ctrl->configure || controller_count >= PWM_MAX_CONTROLLERS)
        return -EINVAL;
    controllers[controller_count++] = ctrl;
    return 0;
}

struct pwm_controller *pwm_get(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

int pwm_config(struct pwm_controller *ctrl, u8 id, u32 period_ns, u32 duty_ns)
{
    struct pwm_channel ch;

    if (!ctrl || id >= ctrl->nchannels || period_ns == 0)
        return -EINVAL;
    if (duty_ns > period_ns)
        return -ERANGE;

    ch.id = id;
    ch.period_ns = period_ns;
    ch.duty_ns = duty_ns;
    ch.inverted = false;
    ch.enabled = false;
    return ctrl->configure(ctrl, &ch);
}

int pwm_set_duty(struct pwm_controller *ctrl, u8 id, u32 duty_ns)
{
    struct pwm_channel ch = {
        .id = id,
        .period_ns = 0,
        .duty_ns = duty_ns,
        .inverted = false,
        .enabled = true,
    };
    return ctrl ? ctrl->configure(ctrl, &ch) : -EINVAL;
}

int pwm_set_percent(struct pwm_controller *ctrl, u8 id, u32 period_ns,
                    u8 percent)
{
    if (!ctrl || period_ns == 0 || percent > 100)
        return -EINVAL;

    u32 duty = (u32)(((u64)period_ns * percent) / 100);
    return pwm_config(ctrl, id, period_ns, duty);
}

int pwm_enable(struct pwm_controller *ctrl, u8 id, bool enable)
{
    if (!ctrl || !ctrl->enable || id >= ctrl->nchannels)
        return -EINVAL;
    return ctrl->enable(ctrl, id, enable);
}
