#ifndef __BT_ARQ_H__
#define __BT_ARQ_H__

#include <core/types.h>
#include <bt/baseband.h>

/*
 * ARQ transmit queue.
 *
 * Packets stay in the queue until the peer acknowledges them in the header's
 * ARQN bit.  A NACK or a timeout increments the retry counter; exhaustion
 * drops the packet and reports a link loss.
 */

struct bb_arq_pkt {
    struct bb_arq_pkt *next;
    u16 handle;
    u16 len;
    u8 seqn;
    u8 retries;
    bool enc_ready;
    u8 data[BB_MAX_ACL_MTU];
    u8 cipher[BB_MAX_ACL_MTU];
};

void bb_arq_init(void);
int bb_arq_push(u16 handle, const u8 *data, u16 len);
struct bb_arq_pkt *bb_arq_head(u16 handle);
void bb_arq_done(struct bb_arq_pkt *pkt);
void bb_arq_ack(u16 handle, u8 arqn);
void bb_arq_nack(struct bb_arq_pkt *pkt);
void bb_arq_flush(u16 handle);
u8 bb_arq_pending(u16 handle);

#endif /* __BT_ARQ_H__ */
