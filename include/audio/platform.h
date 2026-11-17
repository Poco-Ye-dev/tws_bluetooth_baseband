#ifndef __AUDIO_PLATFORM_H__
#define __AUDIO_PLATFORM_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Board level audio hooks.  A chip port fills this table; the framework only
 * calls these functions, so porting audio to a new part means implementing
 * one structure and no framework changes.
 */

enum audio_platform_route {
    AUDIO_PLATFORM_SPEAKER = 0,
    AUDIO_PLATFORM_HEADPHONE,
    AUDIO_PLATFORM_EARPIECE,
    AUDIO_PLATFORM_LINE_OUT,
};

struct audio_platform_ops {
    const char *name;
    int (*enable)(bool on);
    int (*set_route)(enum audio_platform_route route);
    int (*set_volume)(u8 percent);
    int (*set_mic)(bool on);
    u32 (*clock_hz)(void);
};

int audio_platform_register(const struct audio_platform_ops *ops);
const struct audio_platform_ops *audio_platform_get(void);
int audio_platform_enable(bool on);
int audio_platform_route(enum audio_platform_route route);
int audio_platform_volume(u8 percent);
int audio_platform_mic(bool on);
u32 audio_platform_clock(void);

#endif /* __AUDIO_PLATFORM_H__ */
