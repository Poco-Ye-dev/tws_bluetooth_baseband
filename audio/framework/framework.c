#include <audio/framework.h>
#include <audio/common.h>
#include <core/string.h>
#include <core/math.h>

int audio_fw_init(struct audio_framework *fw, u8 channels)
{
    if (!fw)
        return -1;

    memset(fw, 0, sizeof(*fw));
    fw->channels = channels ? channels : 2;
    fw->route = AUDIO_ROUTE_SPEAKER;
    fw->master_q15 = 32767;
    return 0;
}

int audio_fw_add(struct audio_framework *fw, struct audio_stream *stream)
{
    if (!fw || !stream || fw->count >= AUDIO_MAX_STREAMS)
        return -1;
    fw->streams[fw->count++] = stream;
    return 0;
}

int audio_fw_remove(struct audio_framework *fw, struct audio_stream *stream)
{
    for (unsigned int i = 0; i < fw->count; i++) {
        if (fw->streams[i] != stream)
            continue;
        for (unsigned int j = i; j + 1 < fw->count; j++)
            fw->streams[j] = fw->streams[j + 1];
        fw->count--;
        return 0;
    }
    return -1;
}

void audio_fw_set_sink(struct audio_framework *fw, audio_sink_fn sink, void *arg)
{
    if (!fw)
        return;
    fw->sink = sink;
    fw->sink_arg = arg;
}

int audio_fw_set_route(struct audio_framework *fw, enum audio_route route)
{
    if (!fw || route >= AUDIO_ROUTE_COUNT)
        return -1;
    fw->route = route;
    return 0;
}

enum audio_route audio_fw_route(struct audio_framework *fw)
{
    return fw ? fw->route : AUDIO_ROUTE_SPEAKER;
}

int audio_fw_start(struct audio_framework *fw, const char *name)
{
    for (unsigned int i = 0; i < fw->count; i++) {
        struct audio_stream *stream = fw->streams[i];
        if (strcmp(stream->name, name) != 0)
            continue;
        stream->state = AUDIO_STREAM_RUNNING;
        if (stream->ops && stream->ops->start)
            return stream->ops->start(stream);
        return 0;
    }
    return -1;
}

int audio_fw_stop(struct audio_framework *fw, const char *name)
{
    for (unsigned int i = 0; i < fw->count; i++) {
        struct audio_stream *stream = fw->streams[i];
        if (strcmp(stream->name, name) != 0)
            continue;
        stream->state = AUDIO_STREAM_STOPPED;
        if (stream->ops && stream->ops->stop)
            return stream->ops->stop(stream);
        return 0;
    }
    return -1;
}

void audio_fw_set_master(struct audio_framework *fw, s16 gain_q15)
{
    if (fw)
        fw->master_q15 = gain_q15;
}

void audio_fw_mute(struct audio_framework *fw, bool mute)
{
    if (fw)
        fw->mute = mute;
}

int audio_fw_process(struct audio_framework *fw, u32 frames)
{
    u32 samples = frames * fw->channels;
    bool first = true;

    if (!fw || samples > sizeof(fw->mix) / sizeof(fw->mix[0]))
        return -1;

    memset(fw->mix, 0, samples * sizeof(s16));

    for (unsigned int i = 0; i < fw->count; i++) {
        struct audio_stream *stream = fw->streams[i];
        if (stream->state != AUDIO_STREAM_RUNNING || stream->mute)
            continue;
        if (stream->route != fw->route)
            continue;
        if (!stream->ops || !stream->ops->pull)
            continue;

        s16 scratch[2 * 480];
        u32 got = stream->ops->pull(stream, scratch, frames);
        if (!got)
            continue;

        u32 count = got * fw->channels;
        audio_apply_gain(scratch, count, stream->volume_q15 ?
                         stream->volume_q15 : 32767);

        if (first) {
            memcpy(fw->mix, scratch, count * sizeof(s16));
            first = false;
        } else {
            audio_mix_add(fw->mix, scratch, count);
        }
    }

    if (fw->mute)
        return 0;

    audio_apply_gain(fw->mix, samples, fw->master_q15);
    if (fw->sink)
        return fw->sink(fw->mix, frames, fw->channels, fw->sink_arg);
    return 0;
}
