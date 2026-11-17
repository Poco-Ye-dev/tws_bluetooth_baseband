#ifndef __AUDIO_API_H__
#define __AUDIO_API_H__

#include <core/types.h>
#include <audio/framework.h>
#include <audio/platform.h>

/*
 * Product facing audio facade.  It binds the framework, board hooks and
 * volume policy into the handful of calls an application actually needs.
 */

int audio_api_init(u8 channels);
int audio_api_add_stream(struct audio_stream *stream);
int audio_api_start(const char *name);
int audio_api_stop(const char *name);
int audio_api_set_route(enum audio_route route);
int audio_api_set_volume(u8 percent);
int audio_api_process(u32 frames);
struct audio_framework *audio_api_framework(void);

#endif /* __AUDIO_API_H__ */
