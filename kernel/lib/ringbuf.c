#include <core/ringbuf.h>
#include <core/errno.h>
#include <core/string.h>

int ringbuf_init(struct ringbuf *rb, void *storage, u32 capacity)
{
    if (!rb || !storage || capacity < 2)
        return -EINVAL;

    /* Round up to a power of two so the wrap becomes a mask. */
    u32 size = 1;
    while (size < capacity)
        size <<= 1;

    rb->data = storage;
    rb->size = size;
    rb->mask = size - 1;
    rb->head = 0;
    rb->tail = 0;
    return 0;
}

void ringbuf_reset(struct ringbuf *rb)
{
    rb->head = rb->tail = 0;
}

u32 ringbuf_used(const struct ringbuf *rb)
{
    return rb->head - rb->tail;
}

u32 ringbuf_free(const struct ringbuf *rb)
{
    return rb->size - ringbuf_used(rb);
}

bool ringbuf_empty(const struct ringbuf *rb)
{
    return rb->head == rb->tail;
}

bool ringbuf_full(const struct ringbuf *rb)
{
    return ringbuf_free(rb) == 0;
}

u32 ringbuf_write(struct ringbuf *rb, const void *src, u32 len)
{
    const u8 *in = src;
    u32 space = ringbuf_free(rb);
    u32 i;

    if (len > space)
        len = space;

    for (i = 0; i < len; i++) {
        rb->data[(rb->head + i) & rb->mask] = in[i];
    }
    rb->head += len;
    return len;
}

u32 ringbuf_read(struct ringbuf *rb, void *dst, u32 len)
{
    u8 *out = dst;
    u32 avail = ringbuf_used(rb);
    u32 i;

    if (len > avail)
        len = avail;

    for (i = 0; i < len; i++) {
        out[i] = rb->data[(rb->tail + i) & rb->mask];
    }
    rb->tail += len;
    return len;
}

u32 ringbuf_write_ptr(struct ringbuf *rb, u8 **ptr)
{
    u32 pos = rb->head & rb->mask;
    u32 space = ringbuf_free(rb);
    u32 contig = rb->size - pos;

    *ptr = &rb->data[pos];
    return space < contig ? space : contig;
}

void ringbuf_write_put(struct ringbuf *rb, u32 len)
{
    rb->head += len;
}

u32 ringbuf_read_ptr(struct ringbuf *rb, const u8 **ptr)
{
    u32 pos = rb->tail & rb->mask;
    u32 avail = ringbuf_used(rb);
    u32 contig = rb->size - pos;

    *ptr = &rb->data[pos];
    return avail < contig ? avail : contig;
}

void ringbuf_read_put(struct ringbuf *rb, u32 len)
{
    rb->tail += len;
}

bool ringbuf_peek(const struct ringbuf *rb, void *dst, u32 len)
{
    u8 *out = dst;

    if (len > ringbuf_used(rb))
        return false;

    for (u32 i = 0; i < len; i++)
        out[i] = rb->data[(rb->tail + i) & rb->mask];
    return true;
}
