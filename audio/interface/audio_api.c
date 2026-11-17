#include <audio/api.h>
#include <audio/common.h>

static struct audio_framework framework;
static bool initialized;

int audio_api_init(u8 channels)
{
    int ret = audio_fw_init(&framework, channels);
    if (ret)
        return ret;

    initialized = true;
    audio_platform_enable(true);
    audio_platform_route(AUDIO_PLATFORM_SPEAKER);
    return 0;
}

int audio_api_add_stream(struct audio_stream *stream)
{
    return initialized ? audio_fw_add(&framework, stream) : -1;
}

int audio_api_start(const char *name)
{
    return initialized ? audio_fw_start(&framework, name) : -1;
}

int audio_api_stop(const char *name)
{
    return initialized ? audio_fw_stop(&framework, name) : -1;
}

int audio_api_set_route(enum audio_route route)
{
    if (!initialized)
        return -1;

    switch (route) {
    case AUDIO_ROUTE_HEADSET:
        audio_platform_route(AUDIO_PLATFORM_HEADPHONE);
        break;
    case AUDIO_ROUTE_LE:
        audio_platform_route(AUDIO_PLATFORM_SPEAKER);
        break;
    default:
        audio_platform_route(AUDIO_PLATFORM_SPEAKER);
        break;
    }
    return audio_fw_set_route(&framework, route);
}

int audio_api_set_volume(u8 percent)
{
    if (!initialized || percent > 100)
        return -1;

    s16 gain = (s16)(((u32)percent * 32767) / 100);
    audio_fw_set_master(&framework, gain);
    return audio_platform_volume(percent);
}

int audio_api_process(u32 frames)
{
    return initialized ? audio_fw_process(&framework, frames) : -1;
}

struct audio_framework *audio_api_framework(void)
{
    return &framework;
}
