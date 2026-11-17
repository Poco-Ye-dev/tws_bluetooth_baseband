#ifndef __AUDIO_SBC_HW_H__
#define __AUDIO_SBC_HW_H__

#include <core/types.h>

/*
 * SBC decoder interface + simulated hardware implementation.
 *
 * On target the decode is done by the chip's SBC block.  For host testing
 * this module exposes the same API and a simulation that consumes reference
 * PCM, so the A2DP-sink -> TWS pipeline can be exercised without silicon.
 */

struct sbc_frame_info {
    u32 sample_rate;
    u8 channels;
    u8 blocks;
    u8 subbands;
    u8 bitpool;
    u8 mode;
    u16 frame_len;
};

int sbc_parse_header(const u8 *frame, u32 len, struct sbc_frame_info *info);

void sbc_hw_sim_load(const s16 *pcm, u32 samples, u32 sample_rate, u8 channels);
void sbc_hw_sim_reset(void);
int sbc_hw_decode(const u8 *frame, u32 len, s16 *pcm, u32 cap);
u32 sbc_hw_frames_decoded(void);

#endif /* __AUDIO_SBC_HW_H__ */
