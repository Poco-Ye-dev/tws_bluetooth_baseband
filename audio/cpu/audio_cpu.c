#include <audio/cpu.h>

static const struct audio_cpu_ops *cpu_ops;

int audio_cpu_register(const struct audio_cpu_ops *ops)
{
    if (!ops)
        return -1;
    cpu_ops = ops;
    return 0;
}

const struct audio_cpu_ops *audio_cpu_get(void)
{
    return cpu_ops;
}

u32 audio_cpu_clock(void)
{
    return (cpu_ops && cpu_ops->clock_hz) ? cpu_ops->clock_hz() : 0;
}

int audio_cpu_divider(u32 sample_rate)
{
    if (!cpu_ops || !cpu_ops->set_divider)
        return -1;
    return cpu_ops->set_divider(sample_rate);
}

int audio_cpu_codec_init(bool capture)
{
    if (!cpu_ops || !cpu_ops->codec_init)
        return -1;
    return cpu_ops->codec_init(capture);
}

int audio_cpu_trim(void)
{
    if (!cpu_ops || !cpu_ops->codec_trim)
        return -1;
    return cpu_ops->codec_trim();
}
