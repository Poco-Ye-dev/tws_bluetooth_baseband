#ifndef __BT_LE_RING_H__
#define __BT_LE_RING_H__

#include <core/types.h>

/*
 * Single producer / single consumer ring carrying length prefixed packets.
 *
 * TX ring: host writes, LL/ISR reads.  RX ring: ISR writes, LL host reads.
 * Only head is written by the producer and tail by the consumer, so no lock
 * is needed on the single core.
 */

struct le_ring {
    u8 *storage;
    u16 size;
    u16 mask;
    volatile u16 head;
    volatile u16 tail;
};

int le_ring_init(struct le_ring *ring, u8 *storage, u16 size);
void le_ring_reset(struct le_ring *ring);
u16 le_ring_used(const struct le_ring *ring);
u16 le_ring_free(const struct le_ring *ring);
u16 le_ring_push(struct le_ring *ring, const void *data, u16 len);
u16 le_ring_pop(struct le_ring *ring, void *data, u16 cap);

#endif /* __BT_LE_RING_H__ */
