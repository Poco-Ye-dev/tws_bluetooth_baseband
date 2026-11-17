#include <media/container.h>
#include <core/string.h>

#define WAV_RIFF 0x46464952u
#define WAV_WAVE 0x45564157u
#define WAV_FMT  0x20746D66u
#define WAV_DATA 0x61746164u

struct riff_header {
    u32 riff;
    u32 size;
    u32 wave;
};

struct chunk_header {
    u32 id;
    u32 size;
};

struct wav_fmt {
    u16 format;
    u16 channels;
    u32 sample_rate;
    u32 byte_rate;
    u16 block_align;
    u16 bits;
};

int container_parse_wav(const void *data, u32 len, struct audio_stream_info *info)
{
    const struct riff_header *riff = data;
    const u8 *p;
    const u8 *end;

    if (!data || !info || len < sizeof(*riff))
        return -EINVAL;

    if (le32_to_cpu(riff->riff) != WAV_RIFF || le32_to_cpu(riff->wave) != WAV_WAVE)
        return -EINVAL;

    memset(info, 0, sizeof(*info));
    info->container = CONTAINER_WAV;
    info->codec = CODEC_TYPE_PCM;

    p = (const u8 *)data + sizeof(*riff);
    end = (const u8 *)data + len;

    while (p + sizeof(struct chunk_header) <= end) {
        const struct chunk_header *chunk = (const struct chunk_header *)p;
        u32 id = le32_to_cpu(chunk->id);
        u32 size = le32_to_cpu(chunk->size);
        const u8 *body = p + sizeof(*chunk);

        if (body + size > end)
            size = (u32)(end - body);

        if (id == WAV_FMT && size >= sizeof(struct wav_fmt)) {
            const struct wav_fmt *fmt = (const struct wav_fmt *)body;
            info->channels = (u8)le16_to_cpu(fmt->channels);
            info->sample_rate = le32_to_cpu(fmt->sample_rate);
            info->bits = (u8)le16_to_cpu(fmt->bits);
        } else if (id == WAV_DATA) {
            info->data_offset = (u32)(body - (const u8 *)data);
            info->data_size = size;
            return 0;
        }

        p = body + size + (size & 1u);
    }

    return -EPROTO;
}

int container_probe(const void *data, u32 len, struct audio_stream_info *info)
{
    const u8 *p = data;

    if (!data || !info || len < 4)
        return -EINVAL;

    if (le32_to_cpu(*(const u32 *)p) == WAV_RIFF)
        return container_parse_wav(data, len, info);

    memset(info, 0, sizeof(*info));
    if (len > 2 && p[0] == 0xFF && (p[1] & 0xF6) == 0xF0) {
        info->container = CONTAINER_AAC;
        info->codec = CODEC_TYPE_AAC;
    } else if (len > 2 && p[0] == 'I' && p[1] == 'D' && p[2] == '3') {
        info->container = CONTAINER_MP3;
        info->codec = CODEC_TYPE_MP3;
    } else if (len > 4 && p[0] == 'O' && p[1] == 'g' && p[2] == 'g' && p[3] == 'S') {
        info->container = CONTAINER_OGG;
        info->codec = CODEC_TYPE_OPUS;
    } else {
        info->container = CONTAINER_UNKNOWN;
        info->codec = CODEC_TYPE_PCM;
        return -EPROTO;
    }
    return 0;
}

const char *container_name(enum container_format format)
{
    switch (format) {
    case CONTAINER_WAV:  return "wav";
    case CONTAINER_MP3:  return "mp3";
    case CONTAINER_AAC:  return "aac";
    case CONTAINER_OGG:  return "ogg";
    default:             return "unknown";
    }
}
