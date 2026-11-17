#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <core/types.h>

/*
 * Block cipher primitives used by both link layers.
 *
 * AES-128 is implemented here because the baseband needs it for LE CCM and
 * for the EDR key ladder without pulling a crypto library into the firmware.
 */

#define AES_BLOCK_SIZE 16

struct aes128_ctx {
    u8 round_key[176];
};

void aes128_init(struct aes128_ctx *ctx, const u8 key[AES_BLOCK_SIZE]);
void aes128_encrypt(const struct aes128_ctx *ctx, const u8 in[16], u8 out[16]);

/* AES-CTR keystream XOR; counter is the 128 bit block counter. */
void aes128_ctr(const struct aes128_ctx *ctx, const u8 nonce[13],
                u32 counter, const u8 *in, u8 *out, u32 len);

/*
 * LE AES-CCM (L=2, 13 byte nonce, 4 byte MIC).  `out` has room for len+4
 * bytes on encrypt; on decrypt the MIC is verified and an error returned if
 * it does not match.
 */
int aes_ccm_encrypt(const u8 key[16], const u8 nonce[13], const u8 *in,
                    u32 len, u8 *out);
int aes_ccm_decrypt(const u8 key[16], const u8 nonce[13], const u8 *in,
                    u32 len, u8 *out);

/* EDR key ladder stages (simplified AES based derivation). */
void edr_key_derive(const u8 key[16], const u8 rand[16], u8 out[16]);

#endif /* __CRYPTO_H__ */
