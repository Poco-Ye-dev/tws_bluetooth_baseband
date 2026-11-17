#ifndef __AUDIO_CPU_H__
#define __AUDIO_CPU_H__

#include <core/types.h>

/*
 * Per-chip audio port.
 *
 * Keeps the chip specific clock dividers, DAC/ADC trim and codec capabilities
 * behind one table so the framework never includes a CPU header.
 */

struct audio_cpu_ops {
    const char *chip;
    u32 (*clock_hz)(void);
    int (*set_divider)(u32 sample_rate);
    int (*codec_init)(bool capture);
    int (*codec_trim)(void);
    u8 max_channels;
};

int audio_cpu_register(const struct audio_cpu_ops *ops);
const struct audio_cpu_ops *audio_cpu_get(void);
u32 audio_cpu_clock(void);
int audio_cpu_divider(u32 sample_rate);
int audio_cpu_codec_init(bool capture);
int audio_cpu_trim(void);

#endif /* __AUDIO_CPU_H__ */
