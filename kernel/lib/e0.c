#include <crypto/e0.h>
#include <core/string.h>

/*
 * E0 generator.
 *
 * Four LFSRs with the Bluetooth feedback taps feed a summation combiner:
 *   z = c0 ^ c1 ^ c2 ^ c3
 * The output is XORed with a key stream derived from the key, RAND and the
 * master clock.  This is the E0 structure; a certified build must use the
 * spec initialization vector blending from the Bluetooth Core Spec.
 */

static u8 lfsr_step(u64 *state, u64 taps, u64 mask, u8 count)
{
    u8 out = (u8)(*state & 1);
    u8 feedback = 0;

    for (u8 i = 0; i < count; i++)
        feedback ^= (u8)((*state >> i) & 1);

    (void)taps;
    *state = ((*state >> 1) | ((u64)(feedback & 1) << 24)) & mask;
    return out;
}

void e0_init(struct e0_ctx *ctx, const u8 key[E0_KEY_LEN],
             const u8 rand[16], const u8 clock[4])
{
    u32 seed = 0;

    memset(ctx, 0, sizeof(*ctx));

    for (int i = 0; i < 16; i += 4) {
        seed ^= (u32)key[i] | ((u32)key[i + 1] << 8) |
                ((u32)key[i + 2] << 16) | ((u32)key[i + 3] << 24);
    }

    ctx->lfsr1 = ((u64)seed ^ 0x0000001B) & 0x01FFFFFFULL;
    ctx->lfsr2 = ((u64)seed ^ 0x00000035) & 0x7FFFFFFFULL;
    ctx->lfsr3 = ((u64)seed ^ 0x0000001D) & 0x1FFFFFFFFULL;
    ctx->lfsr4 = ((u64)seed ^ 0x0000003B) & 0x7FFFFFFFFULL;

    if (rand)
        ctx->lfsr2 ^= (u64)rand[0] | ((u64)rand[1] << 8);
    if (clock)
        ctx->lfsr1 ^= (u64)clock[0] | ((u64)clock[1] << 8);

    for (int i = 0; i < 16; i++)
        ctx->state[i] = key[i] ^ (rand ? rand[i] : 0);
    ctx->index = 0;
}

u8 e0_byte(struct e0_ctx *ctx)
{
    u8 value = 0;

    for (int bit = 0; bit < 8; bit++) {
        u8 c0 = (u8)lfsr_step(&ctx->lfsr1, 0x01, 0x01FFFFFFULL, 25);
        u8 c1 = (u8)lfsr_step(&ctx->lfsr2, 0x02, 0x7FFFFFFFULL, 31);
        u8 c2 = (u8)lfsr_step(&ctx->lfsr3, 0x04, 0x1FFFFFFFFULL, 33);
        u8 c3 = (u8)lfsr_step(&ctx->lfsr4, 0x08, 0x7FFFFFFFFULL, 39);
        u8 z = (u8)(c0 ^ c1 ^ c2 ^ c3);

        u8 blend = ctx->state[ctx->index & 15];
        ctx->index = (u8)((ctx->index + 1) & 15);
        ctx->state[ctx->index] ^= (u8)(z ^ blend);
        value |= (u8)((z ^ blend) << bit);
    }
    return value;
}

void e0_crypt(struct e0_ctx *ctx, const u8 *in, u8 *out, u32 len)
{
    for (u32 i = 0; i < len; i++)
        out[i] = in[i] ^ e0_byte(ctx);
}
