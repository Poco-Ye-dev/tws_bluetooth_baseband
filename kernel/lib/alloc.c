#include <core/mem.h>
#include <core/string.h>
#include <core/compiler.h>

/*
 * Boundary-tag heap with an address sorted free list.
 *
 * Layout of an allocated block:
 *
 *   +----------------+ <- user pointer
 *   |   block hdr    |    size includes the header, low bit marks "used"
 *   +----------------+
 *   |   payload ...  |
 *   +----------------+
 *
 * A free block additionally stores two list pointers in its payload.  The
 * free list is ordered by address, so freeing a block only has to inspect
 * its immediate neighbours to coalesce.
 */

#ifndef CONFIG_HEAP_SIZE
#define CONFIG_HEAP_SIZE (64 * 1024)
#endif

#define HEAP_ALIGN 8u
#define BLOCK_USED 1u

struct block {
    u32 size;
    u32 used;
    struct block *next_free;
    struct block *prev_free;
};

STATIC_ASSERT(sizeof(struct block) % HEAP_ALIGN == 0, "block header alignment");

static u8 heap_arena[CONFIG_HEAP_SIZE] __aligned(HEAP_ALIGN);

static struct block *free_head;
static size_t heap_used;
static size_t heap_peak;

static inline struct block *block_at(u8 *p)
{
    return (struct block *)p;
}

static inline u8 *payload_of(struct block *b)
{
    return (u8 *)b + sizeof(struct block);
}

static inline u8 *block_end(struct block *b)
{
    return (u8 *)b + b->size;
}

static inline size_t block_payload(struct block *b)
{
    return b->size - sizeof(struct block);
}

static void heap_init(void)
{
    struct block *first = block_at(heap_arena);

    first->size = CONFIG_HEAP_SIZE;
    first->used = 0;
    first->next_free = NULL;
    first->prev_free = NULL;

    free_head = first;
    heap_used = 0;
    heap_peak = 0;
}

static void free_list_insert(struct block *b)
{
    struct block *prev = NULL;
    struct block *cur = free_head;

    while (cur && cur < b) {
        prev = cur;
        cur = cur->next_free;
    }

    b->prev_free = prev;
    b->next_free = cur;
    if (prev)
        prev->next_free = b;
    else
        free_head = b;
    if (cur)
        cur->prev_free = b;
}

static void free_list_remove(struct block *b)
{
    if (b->prev_free)
        b->prev_free->next_free = b->next_free;
    else
        free_head = b->next_free;
    if (b->next_free)
        b->next_free->prev_free = b->prev_free;
    b->next_free = b->prev_free = NULL;
}

static struct block *split(struct block *b, size_t need)
{
    struct block *rest;

    if (block_payload(b) < need + sizeof(struct block) + HEAP_ALIGN)
        return b;

    rest = block_at((u8 *)b + sizeof(struct block) + need);
    rest->size = b->size - sizeof(struct block) - (u32)need;
    rest->used = 0;
    rest->next_free = rest->prev_free = NULL;

    b->size = (u32)(sizeof(struct block) + need);
    return rest;
}

void *kmalloc(size_t size)
{
    struct block *b;

    if (!free_head)
        heap_init();

    if (size == 0)
        return NULL;

    size = (size + HEAP_ALIGN - 1) & ~(size_t)(HEAP_ALIGN - 1);

    for (b = free_head; b; b = b->next_free) {
        if (block_payload(b) < size)
            continue;

        free_list_remove(b);

        if (block_payload(b) >= size + sizeof(struct block) + HEAP_ALIGN) {
            struct block *rest = split(b, size);
            free_list_insert(rest);
        }

        b->used = 1;
        b->next_free = b->prev_free = NULL;

        heap_used += b->size;
        if (heap_used > heap_peak)
            heap_peak = heap_used;

        return payload_of(b);
    }

    return NULL;
}

void *kzalloc(size_t size)
{
    void *p = kmalloc(size);
    if (p)
        memset(p, 0, size);
    return p;
}

void kfree(void *ptr)
{
    struct block *b;
    u8 *next_block;

    if (!ptr)
        return;

    b = block_at((u8 *)ptr - sizeof(struct block));
    if (!b->used)
        return;

    b->used = 0;
    heap_used -= b->size;

    free_list_insert(b);

    next_block = block_end(b);
    if ((u8 *)next_block < heap_arena + CONFIG_HEAP_SIZE) {
        struct block *next = block_at(next_block);
        if (!next->used) {
            free_list_remove(next);
            b->size += next->size;
        }
    }

    if ((u8 *)b > heap_arena) {
        struct block *prev = block_at(heap_arena);
        while (block_end(prev) < (u8 *)b)
            prev = block_at(block_end(prev));
        if (prev != b && !prev->used && block_end(prev) == (u8 *)b) {
            free_list_remove(prev);
            prev->size += b->size;
        }
    }
}

void *krealloc(void *ptr, size_t size)
{
    struct block *b;
    void *out;

    if (!ptr)
        return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }

    b = block_at((u8 *)ptr - sizeof(struct block));
    if (block_payload(b) >= size)
        return ptr;

    out = kmalloc(size);
    if (!out)
        return NULL;
    memcpy(out, ptr, block_payload(b));
    kfree(ptr);
    return out;
}

size_t mem_avail(void)
{
    return CONFIG_HEAP_SIZE - heap_used;
}

size_t mem_peak(void)
{
    return heap_peak;
}

void mem_info(struct mem_info *info)
{
    struct block *b;
    size_t blocks = 0;
    size_t largest = 0;

    if (!info)
        return;

    if (!free_head)
        heap_init();

    for (b = free_head; b; b = b->next_free) {
        size_t payload = block_payload(b);
        blocks++;
        if (payload > largest)
            largest = payload;
    }

    info->total = CONFIG_HEAP_SIZE;
    info->used = heap_used;
    info->peak = heap_peak;
    info->free_blocks = blocks;
    info->largest_free = largest;
}
