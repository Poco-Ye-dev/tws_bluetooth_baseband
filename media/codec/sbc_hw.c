#include <audio/sbc_hw.h>
#include <core/string.h>

static const u32 freq_table[4] = { 16000, 32000, 44100, 48000 };
static const u8 blocks_table[4] = { 4, 8, 12, 16 };

int sbc_parse_header(const u8 *frame, u32 len, struct sbc_frame_info *info)
{
    if (!frame || len < 4 || !info || frame[0] != 0x9C)
        return -1;

    u8 b1 = frame[1];

    info->sample_rate = freq_table[(b1 >> 6) & 0x03];
    info->blocks = blocks_table[(b1 >> 4) & 0x03];
    info->mode = (b1 >> 2) & 0x03;
    info->subbands = (b1 & 0x01) ? 8 : 4;
    info->bitpool = frame[2];
    info->channels = info->mode == 0 ? 1 : 2;

    u32 sf_bits = 4u * info->subbands * info->channels;
    u32 sf_bytes = (sf_bits + 7) / 8;
    u32 audio_bits;

    if (info->mode == 0 || info->mode == 1)
        audio_bits = info->blocks * info->channels * info->bitpool;
    else if (info->mode == 2)
        audio_bits = info->blocks * 2 * info->bitpool;
    else
        audio_bits = info->blocks * 2 * info->bitpool + info->subbands;

    info->frame_len = (u16)(4 + sf_bytes + (audio_bits + 7) / 8);
    return info->frame_len <= len ? 0 : 1;
}

static struct {
    const s16 *pcm;
    u32 samples;
    u32 cursor;
    u32 rate;
    u8 channels;
    u32 frames;
} sim;

void sbc_hw_sim_load(const s16 *pcm, u32 samples, u32 sample_rate, u8 channels)
{
    sim.pcm = pcm;
    sim.samples = samples;
    sim.cursor = 0;
    sim.rate = sample_rate;
    sim.channels = channels;
    sim.frames = 0;
}

void sbc_hw_sim_reset(void)
{
    sim.cursor = 0;
    sim.frames = 0;
}

int sbc_hw_decode(const u8 *frame, u32 len, s16 *pcm, u32 cap)
{
    struct sbc_frame_info info;
    u32 samples;

    if (sbc_parse_header(frame, len, &info))
        return -1;

    samples = (u32)info.blocks * info.subbands * info.channels;
    if (samples > cap)
        samples = cap;
    if (!sim.pcm || sim.cursor + samples > sim.samples)
        return 0;

    memcpy(pcm, &sim.pcm[sim.cursor], samples * sizeof(s16));
    sim.cursor += samples;
    sim.frames++;
    return (int)samples;
}

u32 sbc_hw_frames_decoded(void)
{
    return sim.frames;
}
