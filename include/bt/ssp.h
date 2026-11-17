#ifndef __BT_SSP_H__
#define __BT_SSP_H__

#include <core/types.h>
#include <bt/types.h>

/*
 * Secure Simple Pairing state machine.
 *
 * The protocol exchanges (IO capability, public key, confirm, random, DHKey
 * check) are implemented here.  The P-256 point multiplication and SHA-256
 * are behind hooks so a port can bind a hardware accelerator or a compact
 * software implementation without touching the state machine.
 */

enum ssp_io_cap {
    SSP_IO_DISPLAY_ONLY = 0,
    SSP_IO_DISPLAY_YES_NO,
    SSP_IO_KEYBOARD_ONLY,
    SSP_IO_NO_INPUT_NO_OUTPUT,
    SSP_IO_KEYBOARD_DISPLAY,
};

enum ssp_state {
    SSP_IDLE = 0,
    SSP_WAIT_PUBKEY,
    SSP_WAIT_CONFIRM,
    SSP_WAIT_RANDOM,
    SSP_WAIT_DHKEY,
    SSP_PAIRED,
    SSP_FAILED,
};

enum ssp_opcode {
    SSP_OP_PUBKEY = 0x80,
    SSP_OP_IO_CAP,
    SSP_OP_CONFIRM,
    SSP_OP_RANDOM,
    SSP_OP_NUMBER,
    SSP_OP_DHKEY,
    SSP_OP_FAILED,
};

struct ssp_keys {
    u8 link_key[16];
    u8 rand[16];
    u16 ediv;
    bool authenticated;
    bool valid;
};

struct ssp_crypto {
    /* Fill a 64 byte public key (X||Y) for the local private key. */
    int (*local_pubkey)(u8 pub[64]);
    /* Shared secret: ECDH(local_priv, peer_pub) -> X coordinate. */
    int (*dhkey)(const u8 peer_pub[64], u8 shared[32]);
    /* SHA-256 over the provided chunks. */
    int (*sha256)(const u8 *data, u32 len, u8 out[32]);
};

struct ssp_store {
    int (*save)(const bt_addr *peer, const struct ssp_keys *keys);
    int (*load)(const bt_addr *peer, struct ssp_keys *keys);
};

struct ssp_link;

typedef int (*ssp_send_t)(struct ssp_link *link, u8 opcode,
                          const void *data, u16 len);

struct ssp_link {
    bool used;
    u16 handle;
    bt_addr peer;
    enum ssp_state state;
    enum ssp_io_cap io_cap;
    enum ssp_io_cap peer_io;
    bool mitm;
    bool sc;
    bool peer_mitm;
    u8 local_pub[64];
    u8 peer_pub[64];
    u8 local_confirm[16];
    u8 local_random[16];
    u8 peer_confirm[16];
    u8 peer_random[16];
    u8 shared[32];
    struct ssp_keys keys;
    ssp_send_t send;
    void (*on_paired)(struct ssp_link *link, bool ok);
    void *arg;
};

int ssp_init(const struct ssp_crypto *crypto, const struct ssp_store *store);
struct ssp_link *ssp_link_open(u16 handle, const bt_addr *peer, ssp_send_t send,
                               void (*on_paired)(struct ssp_link *, bool),
                               void *arg);
void ssp_link_close(u16 handle);
int ssp_pair(struct ssp_link *link, enum ssp_io_cap io_cap, bool mitm, bool sc);
int ssp_confirm_reply(struct ssp_link *link, bool accept);
int ssp_passkey_reply(struct ssp_link *link, u32 passkey);
int ssp_cancel(struct ssp_link *link);
void ssp_rx(struct ssp_link *link, u8 opcode, const u8 *data, u16 len);
bool ssp_bonded(struct ssp_link *link);

#endif /* __BT_SSP_H__ */
