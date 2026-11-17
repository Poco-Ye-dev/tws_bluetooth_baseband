#ifndef __MEDIA_ADPCM_H__
#define __MEDIA_ADPCM_H__

#include <core/types.h>

/*
 * IMA ADPCM block codec.
 *
 * 4 bits per sample with a per-channel predictor and step index.  It is the
 * format used for ring tones and low bitrate prompts where a full codec would
 * waste both flash and cycles.
 */

struct adpcm_state {
    s32 predictor;
    s32 step_index;
};

void adpcm_reset(struct adpcm_state *state);
int adpcm_decode(struct adpcm_state *state, const u8 *in, s16 *out, u32 samples);
int adpcm_encode(struct adpcm_state *state, const s16 *in, u8 *out, u32 samples);

#endif /* __MEDIA_ADPCM_H__ */
