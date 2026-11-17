#ifndef __CORE_RINGBUF_H__
#define __CORE_RINGBUF_H__

#include <core/types.h>

/*
 * Byte oriented FIFO for DMA hand-off and UART receive buffers.
 *
 * Single producer / single consumer: the producer only writes `head` and the
 * consumer only writes `tail`, so no lock is needed.  Capacity is rounded up
 * to a power of two to replace the modulo with a mask.
 */

struct ringbuf {
    u8 *data;
    u32 size;
    u32 mask;
    volatile u32 head;
    volatile u32 tail;
};

int ringbuf_init(struct ringbuf *rb, void *storage, u32 capacity);
void ringbuf_reset(struct ringbuf *rb);

u32 ringbuf_free(const struct ringbuf *rb);
u32 ringbuf_used(const struct ringbuf *rb);
bool ringbuf_empty(const struct ringbuf *rb);
bool ringbuf_full(const struct ringbuf *rb);

/* Copy as much as fits; return the number of bytes transferred. */
u32 ringbuf_write(struct ringbuf *rb, const void *src, u32 len);
u32 ringbuf_read(struct ringbuf *rb, void *dst, u32 len);

/* Zero-copy access for DMA: pair each get with the matching put. */
u32 ringbuf_write_ptr(struct ringbuf *rb, u8 **ptr);
void ringbuf_write_put(struct ringbuf *rb, u32 len);
u32 ringbuf_read_ptr(struct ringbuf *rb, const u8 **ptr);
void ringbuf_read_put(struct ringbuf *rb, u32 len);

bool ringbuf_peek(const struct ringbuf *rb, void *dst, u32 len);

#endif /* __CORE_RINGBUF_H__ */
