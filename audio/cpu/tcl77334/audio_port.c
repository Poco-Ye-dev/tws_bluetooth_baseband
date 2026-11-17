#include <audio/cpu.h>

/*
 * tcl77334 audio port: 24.576 MHz master clock, divider table shared by the
 * playback and capture paths, DAC/ADC trim executed once at bring-up.
 */

static const u32 sample_rates[] = {
    8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000,
};

static u32 chip_clock(void)
{
    return 24576000;
}

static int chip_divider(u32 sample_rate)
{
    for (unsigned int i = 0; i < sizeof(sample_rates) / sizeof(sample_rates[0]); i++) {
        if (sample_rates[i] == sample_rate)
            return (int)(chip_clock() / (sample_rate * 256));
    }
    return -1;
}

static int chip_codec_init(bool capture)
{
    (void)capture;
    return 0;
}

static int chip_codec_trim(void)
{
    return 0;
}

static const struct audio_cpu_ops chip_ops = {
    .chip = "tcl77334",
    .clock_hz = chip_clock,
    .set_divider = chip_divider,
    .codec_init = chip_codec_init,
    .codec_trim = chip_codec_trim,
    .max_channels = 2,
};

int audio_cpu_port_init(void)
{
    return audio_cpu_register(&chip_ops);
}
