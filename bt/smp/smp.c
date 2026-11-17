#include <bt/smp.h>
#include <core/errno.h>
#include <core/string.h>

#define SMP_OP_PAIRING_REQ  0x01
#define SMP_OP_PAIRING_RSP  0x02
#define SMP_OP_CONFIRM      0x03
#define SMP_OP_RANDOM       0x04
#define SMP_OP_FAILED       0x05
#define SMP_OP_ENCRYPT_INFO 0x06
#define SMP_OP_MASTER_ID    0x07

static u32 rng_state;

static u32 rng_next(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state;
}

static void smp_send(struct smp *smp, const void *pdu, u16 len)
{
    if (smp->chan)
        l2cap_send(smp->chan, pdu, len);
}

static void smp_fail(struct smp *smp, u8 reason)
{
    u8 pdu[2] = { SMP_OP_FAILED, reason };
    smp_send(smp, pdu, sizeof(pdu));
    smp->state = SMP_FAILED;
    if (smp->on_paired)
        smp->on_paired(&smp->peer, false, smp->arg);
}

static void smp_complete(struct smp *smp)
{
    smp->state = SMP_PAIRED;
    smp->bonded = true;
    smp->keys.valid = true;
    if (smp->store && smp->store->save)
        smp->store->save(&smp->peer, &smp->keys);
    if (smp->on_paired)
        smp->on_paired(&smp->peer, true, smp->arg);
}

int smp_init(struct smp *smp, const struct smp_store_ops *store)
{
    if (!smp)
        return -EINVAL;

    memset(smp, 0, sizeof(*smp));
    smp->state = SMP_IDLE;
    smp->store = store;
    smp->io_cap = SMP_IO_NO_INPUT_NO_OUTPUT;
    rng_state = (u32)(uptr)smp ^ 0x9E3779B9u;
    return 0;
}

int smp_pair(struct smp *smp, struct l2cap_chan *chan, enum smp_io_cap io_cap,
             bool mitm)
{
    u8 pdu[7];

    if (!smp || !chan)
        return -EINVAL;

    if (smp->store && smp->store->load)
        smp->store->load(&smp->peer, &smp->keys);

    smp->chan = chan;
    smp->io_cap = io_cap;
    smp->mitm = mitm;
    smp->state = SMP_WAIT_PAIRING_RSP;

    pdu[0] = SMP_OP_PAIRING_REQ;
    pdu[1] = (u8)io_cap;
    pdu[2] = 0x00;
    pdu[3] = mitm ? 0x04 : 0x00;
    pdu[4] = 16;
    pdu[5] = 0x0F;
    pdu[6] = 0x0F;
    smp_send(smp, pdu, sizeof(pdu));
    return 0;
}

int smp_cancel(struct smp *smp)
{
    if (!smp)
        return -EINVAL;
    smp->state = SMP_IDLE;
    return 0;
}

static void handle_pairing_rsp(struct smp *smp, const u8 *data, u16 len)
{
    if (len < 1) {
        smp_fail(smp, 0x03);
        return;
    }
    smp->io_cap = (enum smp_io_cap)data[0];
    smp->state = SMP_WAIT_CONFIRM;

    u8 pdu[17];
    pdu[0] = SMP_OP_CONFIRM;
    for (int i = 0; i < 16; i++)
        pdu[1 + i] = (u8)rng_next();
    smp_send(smp, pdu, sizeof(pdu));
}

static void handle_random(struct smp *smp)
{
    for (int i = 0; i < 16; i++)
        smp->keys.ltk[i] = (u8)rng_next();
    smp_complete(smp);
}

void smp_rx(struct smp *smp, struct bt_buf *buf)
{
    u8 opcode;

    if (!smp || !buf)
        return;

    opcode = bt_buf_get_u8(buf);
    u16 len = bt_buf_len(buf);
    const u8 *data = bt_buf_pull(buf, len);

    switch (opcode) {
    case SMP_OP_PAIRING_REQ:
        smp->state = SMP_WAIT_PAIRING_REQ;
        break;
    case SMP_OP_PAIRING_RSP:
        handle_pairing_rsp(smp, data, len);
        break;
    case SMP_OP_CONFIRM:
        smp->state = SMP_WAIT_RANDOM;
        {
            u8 pdu[17] = { SMP_OP_RANDOM };
            for (int i = 0; i < 16; i++)
                pdu[1 + i] = (u8)rng_next();
            smp_send(smp, pdu, sizeof(pdu));
        }
        break;
    case SMP_OP_RANDOM:
        handle_random(smp);
        break;
    case SMP_OP_FAILED:
        smp->state = SMP_FAILED;
        if (smp->on_paired)
            smp->on_paired(&smp->peer, false, smp->arg);
        break;
    default:
        break;
    }

    bt_buf_put(buf);
}

bool smp_bonded(const struct smp *smp)
{
    return smp && smp->bonded && smp->keys.valid;
}
