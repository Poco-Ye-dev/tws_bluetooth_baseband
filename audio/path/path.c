#include <audio/path.h>
#include <core/string.h>

static const char *const path_names[AUDIO_PATH_COUNT] = {
    "music", "voice", "prompt",
};

int audio_path_init(struct audio_path *path, enum audio_path_id id,
                    struct i2s_port *port)
{
    if (!path || id >= AUDIO_PATH_COUNT)
        return -EINVAL;

    memset(path, 0, sizeof(*path));
    path->id = id;
    path->name = path_names[id];
    path->port = port;
    return pipeline_init(&path->pipe);
}

int audio_path_open(struct audio_path *path,
                    const struct audio_path_config *config)
{
    struct i2s_config cfg;

    if (!path || !config || !config->sample_rate || !config->channels)
        return -EINVAL;

    path->config = *config;

    if (!path->port)
        return 0;

    cfg.sample_rate = config->sample_rate;
    cfg.channels = config->channels;
    cfg.bits = config->bits ? config->bits : 16;
    cfg.format = I2S_FMT_I2S;
    cfg.master = true;
    cfg.capture = false;

    int ret = i2s_configure(path->port, &cfg);
    if (ret)
        return ret;

    path->opened = true;
    return 0;
}

int audio_path_start(struct audio_path *path)
{
    if (!path || !path->opened)
        return -EINVAL;
    if (path->running)
        return 0;

    if (path->port) {
        int ret = i2s_start(path->port);
        if (ret)
            return ret;
    }

    path->pipe.running = true;
    path->running = true;
    return 0;
}

int audio_path_stop(struct audio_path *path)
{
    if (!path)
        return -EINVAL;

    if (path->port && path->running)
        i2s_stop(path->port);

    path->pipe.running = false;
    path->running = false;
    return 0;
}

int audio_path_write(struct audio_path *path, const void *data, u32 frames)
{
    if (!path || !path->running || !path->port)
        return -EPERM;
    return i2s_write(path->port, data, frames);
}

int audio_path_read(struct audio_path *path, void *data, u32 frames)
{
    if (!path || !path->running || !path->port)
        return -EPERM;
    return i2s_read(path->port, data, frames);
}

struct pipeline *audio_path_pipeline(struct audio_path *path)
{
    return path ? &path->pipe : NULL;
}
