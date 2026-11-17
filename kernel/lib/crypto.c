#include <crypto/aes.h>
#include <core/string.h>

/*
 * AES-128 (FIPS-197) with a compact S-box.  The table is the standard one;
 * the implementation favours size over table-free key expansion because the
 * baseband only derives keys and encrypts a handful of blocks per event.
 */

static const u8 sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
    0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
    0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
    0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
    0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
    0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
    0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
    0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
    0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
    0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
    0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
    0xB0, 0x54, 0xBB, 0x16,
};

static const u8 rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36,
};

static u8 xtime(u8 x)
{
    return (u8)((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00));
}

void aes128_init(struct aes128_ctx *ctx, const u8 key[16])
{
    memcpy(ctx->round_key, key, 16);

    for (int i = 4; i < 44; i++) {
        u8 temp[4];
        memcpy(temp, &ctx->round_key[(i - 1) * 4], 4);

        if (i % 4 == 0) {
            u8 t = temp[0];
            temp[0] = sbox[temp[1]] ^ rcon[i / 4];
            temp[1] = sbox[temp[2]];
            temp[2] = sbox[temp[3]];
            temp[3] = sbox[t];
        }

        for (int j = 0; j < 4; j++)
            ctx->round_key[i * 4 + j] = ctx->round_key[(i - 4) * 4 + j] ^ temp[j];
    }
}

static void add_round_key(u8 state[16], const u8 *rk)
{
    for (int i = 0; i < 16; i++)
        state[i] ^= rk[i];
}

static void sub_bytes(u8 state[16])
{
    for (int i = 0; i < 16; i++)
        state[i] = sbox[state[i]];
}

static void shift_rows(u8 s[16])
{
    u8 t;

    t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
    t = s[2]; s[2] = s[10]; s[10] = t;
    t = s[6]; s[6] = s[14]; s[14] = t;
    t = s[3]; s[3] = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = t;
}

static void mix_columns(u8 s[16])
{
    for (int c = 0; c < 4; c++) {
        u8 *col = &s[c * 4];
        u8 a = col[0], b = col[1], d = col[2], e = col[3];
        col[0] = (u8)(xtime(a) ^ (xtime(b) ^ b) ^ d ^ e);
        col[1] = (u8)(a ^ xtime(b) ^ (xtime(d) ^ d) ^ e);
        col[2] = (u8)(a ^ b ^ xtime(d) ^ (xtime(e) ^ e));
        col[3] = (u8)((xtime(a) ^ a) ^ b ^ d ^ xtime(e));
    }
}

void aes128_encrypt(const struct aes128_ctx *ctx, const u8 in[16], u8 out[16])
{
    u8 state[16];

    memcpy(state, in, 16);
    add_round_key(state, ctx->round_key);

    for (int round = 1; round <= 10; round++) {
        sub_bytes(state);
        shift_rows(state);
        if (round != 10)
            mix_columns(state);
        add_round_key(state, &ctx->round_key[round * 16]);
    }

    memcpy(out, state, 16);
}

void aes128_ctr(const struct aes128_ctx *ctx, const u8 nonce[13],
                u32 counter, const u8 *in, u8 *out, u32 len)
{
    u8 block[16];
    u8 stream[16];

    memcpy(block, nonce, 13);
    block[13] = (u8)(counter >> 16);
    block[14] = (u8)(counter >> 8);
    block[15] = (u8)counter;

    for (u32 i = 0; i < len; i++) {
        if (i % 16 == 0) {
            aes128_encrypt(ctx, block, stream);
            counter++;
            block[13] = (u8)(counter >> 16);
            block[14] = (u8)(counter >> 8);
            block[15] = (u8)counter;
        }
        out[i] = in[i] ^ stream[i % 16];
    }
}

static void ccm_mac(const struct aes128_ctx *ctx, const u8 *data, u32 len,
                    u8 mac[16])
{
    u8 block[16];

    memset(block, 0, sizeof(block));
    mac[0] = 0x49;                  /* Adata=0, M=4, L=2 */
    memset(mac + 1, 0, 13);
    mac[15] = (u8)(len >> 8);
    mac[14] = (u8)len;

    u8 x[16];
    aes128_encrypt(ctx, mac, x);

    u8 b0[16];
    memcpy(b0, mac, 16);

    for (u32 off = 0; off < len; off += 16) {
        u32 n = len - off > 16 ? 16 : len - off;
        memset(block, 0, 16);
        memcpy(block, data + off, n);
        for (int i = 0; i < 16; i++)
            block[i] ^= x[i];
        aes128_encrypt(ctx, block, x);
    }

    memcpy(mac, x, 16);
    (void)b0;
}

int aes_ccm_encrypt(const u8 key[16], const u8 nonce[13], const u8 *in,
                    u32 len, u8 *out)
{
    struct aes128_ctx ctx;
    u8 mac[16];
    u8 s0[16];
    u8 a0[16];

    if (!in || !out)
        return -1;

    aes128_init(&ctx, key);
    aes128_ctr(&ctx, nonce, 1, in, out, len);
    ccm_mac(&ctx, in, len, mac);

    memset(a0, 0, 16);
    a0[0] = 0x01;
    memcpy(a0 + 1, nonce, 13);
    a0[14] = 0;
    a0[15] = 0;
    aes128_encrypt(&ctx, a0, s0);

    for (int i = 0; i < 4; i++)
        out[len + i] = mac[i] ^ s0[i];
    return 0;
}

int aes_ccm_decrypt(const u8 key[16], const u8 nonce[13], const u8 *in,
                    u32 len, u8 *out)
{
    struct aes128_ctx ctx;
    u8 mac[16];
    u8 s0[16];
    u8 a0[16];
    u32 plain_len = len >= 4 ? len - 4 : 0;

    if (!in || !out || len < 4)
        return -1;

    aes128_init(&ctx, key);
    aes128_ctr(&ctx, nonce, 1, in, out, plain_len);
    ccm_mac(&ctx, out, plain_len, mac);

    memset(a0, 0, 16);
    a0[0] = 0x01;
    memcpy(a0 + 1, nonce, 13);
    aes128_encrypt(&ctx, a0, s0);

    for (int i = 0; i < 4; i++) {
        if ((mac[i] ^ s0[i]) != in[plain_len + i])
            return -2;
    }
    return 0;
}

void edr_key_derive(const u8 key[16], const u8 rand[16], u8 out[16])
{
    struct aes128_ctx ctx;
    u8 block[16];

    aes128_init(&ctx, key);
    for (int i = 0; i < 16; i++)
        block[i] = rand[i] ^ key[i];
    aes128_encrypt(&ctx, block, out);
}
