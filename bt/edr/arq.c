#include "arq.h"
#include <core/string.h>

#define ARQ_POOL 16

static struct bb_arq_pkt pool[ARQ_POOL];
static bool used[ARQ_POOL];

void bb_arq_init(void)
{
    memset(pool, 0, sizeof(pool));
    memset(used, 0, sizeof(used));
}

static struct bb_arq_pkt *alloc(void)
{
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (!used[i]) {
            used[i] = true;
            memset(&pool[i], 0, sizeof(pool[i]));
            return &pool[i];
        }
    }
    return NULL;
}

int bb_arq_push(u16 handle, const u8 *data, u16 len)
{
    struct bb_arq_pkt *pkt;

    if (!data || len > BB_MAX_ACL_MTU)
        return -1;

    pkt = alloc();
    if (!pkt)
        return -2;

    pkt->handle = handle;
    pkt->len = len;
    pkt->enc_ready = false;
    memcpy(pkt->data, data, len);

    struct bb_arq_pkt **at = &pool[0].next;
    (void)at;

    /* Append behind the last queued packet for this handle. */
    struct bb_arq_pkt *last = NULL;
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (used[i] && &pool[i] != pkt && pool[i].handle == handle)
            last = &pool[i];
        if (last && last->next == pkt)
            break;
    }
    pkt->next = NULL;
    if (last)
        last->next = pkt;
    return 0;
}

struct bb_arq_pkt *bb_arq_head(u16 handle)
{
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (used[i] && pool[i].handle == handle)
            return &pool[i];
    }
    return NULL;
}

void bb_arq_done(struct bb_arq_pkt *pkt)
{
    if (!pkt)
        return;
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (&pool[i] == pkt) {
            used[i] = false;
            memset(pkt, 0, sizeof(*pkt));
            return;
        }
    }
}

void bb_arq_ack(u16 handle, u8 arqn)
{
    struct bb_arq_pkt *pkt = bb_arq_head(handle);

    if (pkt && pkt->seqn == arqn)
        bb_arq_done(pkt);
}

void bb_arq_nack(struct bb_arq_pkt *pkt)
{
    if (pkt)
        pkt->retries++;
}

void bb_arq_flush(u16 handle)
{
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (used[i] && pool[i].handle == handle)
            bb_arq_done(&pool[i]);
    }
}

u8 bb_arq_pending(u16 handle)
{
    u8 count = 0;
    for (unsigned int i = 0; i < ARQ_POOL; i++) {
        if (used[i] && pool[i].handle == handle)
            count++;
    }
    return count;
}
