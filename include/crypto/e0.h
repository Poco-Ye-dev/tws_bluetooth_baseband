#ifndef __CRYPTO_E0_H__
#define __CRYPTO_E0_H__

#include <core/types.h>

/*
 * E0 stream cipher interface.
 *
 * E0 combines four LFSRs through a summation generator and a 16 state
 * blender.  The registers here follow the published E0 structure; the key
 * loading uses the derived key material directly.
 */

#define E0_KEY_LEN 16

struct e0_ctx {
    u64 lfsr1;
    u64 lfsr2;
    u64 lfsr3;
    u64 lfsr4;
    u8 state[16];
    u8 index;
};

void e0_init(struct e0_ctx *ctx, const u8 key[E0_KEY_LEN],
             const u8 rand[16], const u8 clock[4]);
u8 e0_byte(struct e0_ctx *ctx);
void e0_crypt(struct e0_ctx *ctx, const u8 *in, u8 *out, u32 len);

#endif /* __CRYPTO_E0_H__ */
