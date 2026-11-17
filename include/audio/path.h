#ifndef __AUDIO_PATH_H__
#define __AUDIO_PATH_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/pipeline.h>
#include <driver/i2s.h>

/*
 * An audio path binds an I2S port, a sample format and an optional effect
 * pipeline into a named stream (music, voice, prompt, ...).
 */

enum audio_path_id {
    AUDIO_PATH_MUSIC = 0,
    AUDIO_PATH_VOICE,
    AUDIO_PATH_PROMPT,
    AUDIO_PATH_COUNT,
};

struct audio_path_config {
    u32 sample_rate;
    u8 channels;
    u8 bits;
    u32 buffer_ms;
};

struct audio_path {
    const char *name;
    enum audio_path_id id;
    struct audio_path_config config;
    struct i2s_port *port;
    struct pipeline pipe;
    bool opened;
    bool running;
};

int audio_path_init(struct audio_path *path, enum audio_path_id id,
                    struct i2s_port *port);
int audio_path_open(struct audio_path *path,
                    const struct audio_path_config *config);
int audio_path_start(struct audio_path *path);
int audio_path_stop(struct audio_path *path);
int audio_path_write(struct audio_path *path, const void *data, u32 frames);
int audio_path_read(struct audio_path *path, void *data, u32 frames);
struct pipeline *audio_path_pipeline(struct audio_path *path);

#endif /* __AUDIO_PATH_H__ */
