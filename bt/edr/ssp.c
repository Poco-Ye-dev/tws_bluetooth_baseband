#include <bt/ssp.h>
#include <crypto/aes.h>
#include <core/string.h>
#include <core/errno.h>
#include <bt/lmp.h>

#define SSP_MAX_LINKS 8
#define SSP_AES_OFFSET "bt ssp1"

static struct ssp_crypto crypto;
static struct ssp_store store;
static struct ssp_link links[SSP_MAX_LINKS];

int ssp_init(const struct ssp_crypto *c, const struct ssp_store *s)
{
    memset(links, 0, sizeof(links));
    if (c)
        crypto = *c;
    if (s)
        store = *s;
    return 0;
}

struct ssp_link *ssp_link_open(u16 handle, const bt_addr *peer, ssp_send_t send,
                               void (*on_paired)(struct ssp_link *, bool),
                               void *arg)
{
    for (unsigned int i = 0; i < SSP_MAX_LINKS; i++) {
        struct ssp_link *link = &links[i];
        if (link->used)
            continue;
        memset(link, 0, sizeof(*link));
        link->used = true;
        link->handle = handle;
        link->peer = *peer;
        link->send = send;
        link->on_paired = on_paired;
        link->arg = arg;
        link->state = SSP_IDLE;
        return link;
    }
    return NULL;
}

void ssp_link_close(u16 handle)
{
    for (unsigned int i = 0; i < SSP_MAX_LINKS; i++) {
        if (links[i].used && links[i].handle == handle)
            memset(&links[i], 0, sizeof(links[i]));
    }
}

static int send(struct ssp_link *link, u8 opcode, const void *data, u16 len)
{
    return (link && link->send) ? link->send(link, opcode, data, len) : -1;
}

static void fail(struct ssp_link *link, bool notify)
{
    if (!link)
        return;
    link->state = SSP_FAILED;
    if (notify)
        send(link, SSP_OP_FAILED, NULL, 0);
    if (link->on_paired)
        link->on_paired(link, false);
}

/* Fallback hash used when no SHA-256 hook is bound: ciphered transcript. */
static void fallback_hash(const u8 *data, u32 len, u8 out[16])
{
    struct aes128_ctx ctx;
    u8 block[16];

    memset(block, 0, sizeof(block));
    aes128_init(&ctx, data);
    for (u32 i = 0; i < len; i++) {
        block[i & 15] ^= data[i];
        if ((i & 15) == 15)
            aes128_encrypt(&ctx, block, block);
    }
    aes128_encrypt(&ctx, block, out);
}

static void compute_confirm(struct ssp_link *link)
{
    u8 transcript[16 + 64 + 64 + 16];
    u32 pos = 0;

    memcpy(&transcript[pos], &link->peer.b, 6);
    pos += 6;
    memcpy(&transcript[pos], link->local_pub, 64);
    pos += 64;
    memcpy(&transcript[pos], link->peer_pub, 64);
    pos += 64;
    memcpy(&transcript[pos], link->local_random, 16);
    pos += 16;

    if (crypto.sha256 && link->sc) {
        u8 digest[32];
        crypto.sha256(transcript, pos, digest);
        memcpy(link->local_confirm, digest, 16);
    } else {
        fallback_hash(transcript, pos, link->local_confirm);
    }
}

int ssp_pair(struct ssp_link *link, enum ssp_io_cap io_cap, bool mitm, bool sc)
{
    u8 io[3];

    if (!link)
        return -EINVAL;

    link->io_cap = io_cap;
    link->mitm = mitm;
    link->sc = sc;
    link->state = SSP_WAIT_PUBKEY;

    if (crypto.local_pubkey) {
        if (crypto.local_pubkey(link->local_pub))
            return -EIO;
    } else {
        memset(link->local_pub, 0xA5, sizeof(link->local_pub));
    }

    /* Out of band: none; authentication requirement encodes MITM/SC. */
    io[0] = (u8)io_cap;
    io[1] = 0;
    io[2] = (u8)((mitm ? 0x04 : 0) | (sc ? 0x08 : 0));

    send(link, SSP_OP_IO_CAP, io, sizeof(io));
    send(link, SSP_OP_PUBKEY, link->local_pub, sizeof(link->local_pub));
    return 0;
}

int ssp_confirm_reply(struct ssp_link *link, bool accept)
{
    if (!link || link->state != SSP_WAIT_CONFIRM)
        return -EINVAL;

    if (!accept)
        return ssp_cancel(link);

    send(link, SSP_OP_CONFIRM, link->local_confirm, 16);
    link->state = SSP_WAIT_RANDOM;
    return 0;
}

int ssp_passkey_reply(struct ssp_link *link, u32 passkey)
{
    u8 number[16];

    if (!link || link->state != SSP_WAIT_CONFIRM)
        return -EINVAL;

    memset(number, 0, sizeof(number));
    for (int i = 0; i < 4; i++)
        number[i] = (u8)(passkey >> (8 * i));

    send(link, SSP_OP_NUMBER, number, sizeof(number));
    link->state = SSP_WAIT_RANDOM;
    return 0;
}

int ssp_cancel(struct ssp_link *link)
{
    if (!link)
        return -EINVAL;
    fail(link, true);
    return 0;
}

static void complete(struct ssp_link *link)
{
    u8 seed[32];

    memcpy(seed, link->shared, 16);
    memcpy(seed + 16, link->peer_random, 16);
    edr_key_derive(seed, link->local_random, link->keys.link_key);
    link->keys.valid = true;
    link->keys.authenticated = link->mitm;

    struct lmp_link *lmp = lmp_link_find(link->handle);
    if (lmp)
        lmp_set_link_key(lmp, link->keys.link_key);
    link->state = SSP_PAIRED;

    if (store.save)
        store.save(&link->peer, &link->keys);
    if (link->on_paired)
        link->on_paired(link, true);
}

void ssp_rx(struct ssp_link *link, u8 opcode, const u8 *data, u16 len)
{
    if (!link)
        return;

    switch (opcode) {
    case SSP_OP_IO_CAP:
        if (len >= 3) {
            link->peer_io = (enum ssp_io_cap)data[0];
            link->peer_mitm = (data[2] & 0x04) != 0;
        }
        break;
    case SSP_OP_PUBKEY:
        if (len >= 64) {
            memcpy(link->peer_pub, data, 64);
            if (crypto.dhkey) {
                if (crypto.dhkey(link->peer_pub, link->shared)) {
                    fail(link, true);
                    return;
                }
            } else {
                edr_key_derive(link->peer_pub, link->local_pub, link->shared);
            }
            compute_confirm(link);
            link->state = SSP_WAIT_CONFIRM;
        }
        break;
    case SSP_OP_CONFIRM:
        if (len >= 16) {
            memcpy(link->peer_confirm, data, 16);
            send(link, SSP_OP_RANDOM, link->local_random, 16);
            link->state = SSP_WAIT_RANDOM;
        }
        break;
    case SSP_OP_RANDOM:
        if (len >= 16) {
            memcpy(link->peer_random, data, 16);
            send(link, SSP_OP_DHKEY, link->shared, 16);
            link->state = SSP_WAIT_DHKEY;
        }
        break;
    case SSP_OP_DHKEY:
        complete(link);
        break;
    case SSP_OP_NUMBER:
        if (len >= 16) {
            send(link, SSP_OP_CONFIRM, link->local_confirm, 16);
            link->state = SSP_WAIT_RANDOM;
        }
        break;
    case SSP_OP_FAILED:
        fail(link, false);
        break;
    default:
        break;
    }
}

bool ssp_bonded(struct ssp_link *link)
{
    return link && link->state == SSP_PAIRED && link->keys.valid;
}
