#include <audio/platform.h>

static const struct audio_platform_ops *ops;

int audio_platform_register(const struct audio_platform_ops *new_ops)
{
    if (!new_ops)
        return -EINVAL;
    ops = new_ops;
    return 0;
}

const struct audio_platform_ops *audio_platform_get(void)
{
    return ops;
}

int audio_platform_enable(bool on)
{
    return (ops && ops->enable) ? ops->enable(on) : -ENOSYS;
}

int audio_platform_route(enum audio_platform_route route)
{
    return (ops && ops->set_route) ? ops->set_route(route) : -ENOSYS;
}

int audio_platform_volume(u8 percent)
{
    return (ops && ops->set_volume) ? ops->set_volume(percent) : -ENOSYS;
}

int audio_platform_mic(bool on)
{
    return (ops && ops->set_mic) ? ops->set_mic(on) : -ENOSYS;
}

u32 audio_platform_clock(void)
{
    return (ops && ops->clock_hz) ? ops->clock_hz() : 0;
}
