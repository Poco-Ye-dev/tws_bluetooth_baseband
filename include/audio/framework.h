#ifndef __AUDIO_FRAMEWORK_H__
#define __AUDIO_FRAMEWORK_H__

#include <core/types.h>
#include <media/buffer.h>

/*
 * Audio graph: streams are pulled, mixed and pushed to one sink.
 *
 * Each stream declares a route; only streams on the active route contribute
 * to the mix.  Volume is applied per stream and again at the master stage so
 * ducking can be implemented without touching stream code.
 */

#define AUDIO_MAX_STREAMS 8

enum audio_route {
    AUDIO_ROUTE_SPEAKER = 0,
    AUDIO_ROUTE_HEADSET,
    AUDIO_ROUTE_USB,
    AUDIO_ROUTE_LE,
    AUDIO_ROUTE_COUNT,
};

enum audio_stream_state {
    AUDIO_STREAM_STOPPED = 0,
    AUDIO_STREAM_RUNNING,
    AUDIO_STREAM_PAUSED,
};

struct audio_stream;

struct audio_stream_ops {
    int (*start)(struct audio_stream *stream);
    int (*stop)(struct audio_stream *stream);
    u32 (*pull)(struct audio_stream *stream, s16 *dst, u32 frames);
};

struct audio_stream {
    char name[16];
    enum audio_route route;
    enum audio_stream_state state;
    s16 volume_q15;
    bool mute;
    const struct audio_stream_ops *ops;
    void *arg;
};

typedef int (*audio_sink_fn)(const s16 *data, u32 frames, u8 channels, void *arg);

struct audio_framework {
    struct audio_stream *streams[AUDIO_MAX_STREAMS];
    u8 count;
    enum audio_route route;
    s16 master_q15;
    bool mute;
    u8 channels;
    audio_sink_fn sink;
    void *sink_arg;
    s16 mix[2 * 480];
};

int audio_fw_init(struct audio_framework *fw, u8 channels);
int audio_fw_add(struct audio_framework *fw, struct audio_stream *stream);
int audio_fw_remove(struct audio_framework *fw, struct audio_stream *stream);
void audio_fw_set_sink(struct audio_framework *fw, audio_sink_fn sink, void *arg);
int audio_fw_set_route(struct audio_framework *fw, enum audio_route route);
enum audio_route audio_fw_route(struct audio_framework *fw);
int audio_fw_start(struct audio_framework *fw, const char *name);
int audio_fw_stop(struct audio_framework *fw, const char *name);
void audio_fw_set_master(struct audio_framework *fw, s16 gain_q15);
void audio_fw_mute(struct audio_framework *fw, bool mute);
int audio_fw_process(struct audio_framework *fw, u32 frames);

#endif /* __AUDIO_FRAMEWORK_H__ */
