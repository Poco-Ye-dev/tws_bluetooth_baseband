#include <bt/le_ring.h>
#include <core/string.h>

int le_ring_init(struct le_ring *ring, u8 *storage, u16 size)
{
    u16 pow2 = 1;

    if (!ring || !storage || size < 64)
        return -1;

    while (pow2 < size)
        pow2 <<= 1;

    ring->storage = storage;
    ring->size = pow2;
    ring->mask = (u16)(pow2 - 1);
    ring->head = 0;
    ring->tail = 0;
    return 0;
}

void le_ring_reset(struct le_ring *ring)
{
    if (ring)
        ring->head = ring->tail = 0;
}

u16 le_ring_used(const struct le_ring *ring)
{
    return (u16)(ring->head - ring->tail);
}

u16 le_ring_free(const struct le_ring *ring)
{
    return (u16)(ring->size - le_ring_used(ring));
}

u16 le_ring_push(struct le_ring *ring, const void *data, u16 len)
{
    const u8 *src = data;
    u16 total = (u16)(len + 2);

    if (!ring || !data || total > le_ring_free(ring))
        return 0;

    u16 h = ring->head;
    ring->storage[(h) & ring->mask] = (u8)(len & 0xFF);
    ring->storage[(h + 1) & ring->mask] = (u8)(len >> 8);

    for (u16 i = 0; i < len; i++)
        ring->storage[(h + 2 + i) & ring->mask] = src[i];

    ring->head = (u16)(h + total);
    return len;
}

u16 le_ring_pop(struct le_ring *ring, void *data, u16 cap)
{
    u8 *dst = data;
    u16 h, t, len;

    if (!ring || !data || le_ring_used(ring) < 2)
        return 0;

    t = ring->tail;
    h = ring->head;
    (void)h;

    len = (u16)(ring->storage[t & ring->mask] |
                (ring->storage[(t + 1) & ring->mask] << 8));

    if (len + 2 > le_ring_used(ring) || len > cap)
        return 0;

    for (u16 i = 0; i < len; i++)
        dst[i] = ring->storage[(t + 2 + i) & ring->mask];

    ring->tail = (u16)(t + len + 2);
    return len;
}
