#ifndef __BT_FRAME_H__
#define __BT_FRAME_H__

#include <core/types.h>

/*
 * EDR control frame queue (the 排帧 layer).
 *
 * Frames are queued with a priority and dispatched one per slot.  The queue
 * owns the frame lifetime; the baseband only asks for the next frame for a
 * link and reports the ARQN it observed.
 */

#define EDR_FRAME_MAX 16
#define EDR_FRAME_PAYLOAD 1024

enum frame_prio {
    FRAME_PRIO_LOW = 0,
    FRAME_PRIO_NORMAL,
    FRAME_PRIO_HIGH,
};

struct ctrl_frame {
    struct ctrl_frame *next;
    u16 handle;
    u8 lt_addr;
    u8 type;
    u8 seqn;
    u8 arqn;
    u8 retries;
    u8 prio;
    u16 len;
    u32 slot;
    bool used;
    u8 data[EDR_FRAME_PAYLOAD];
};

void edr_frame_init(void);
struct ctrl_frame *edr_frame_alloc(u16 handle, u8 lt_addr);
void edr_frame_free(struct ctrl_frame *frame);
int edr_frame_enqueue(struct ctrl_frame *frame, u8 prio);
struct ctrl_frame *edr_frame_dequeue(u16 handle);
struct ctrl_frame *edr_frame_peek(u16 handle);
void edr_frame_ack(u16 handle, u8 arqn);
void edr_frame_retry(struct ctrl_frame *frame);
void edr_frame_flush(u16 handle);
u8 edr_frame_pending(u16 handle);
int edr_frame_afh_remap(const u8 *frq_table, const u8 *channel_map,
                        u8 *out_map);

#endif /* __BT_FRAME_H__ */
