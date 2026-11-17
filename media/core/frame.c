#include <media/frame.h>
#include <core/string.h>

int frame_pool_init(struct frame_pool *pool, struct audio_frame *frames,
                    u8 *storage, u16 count, u16 frame_bytes)
{
    if (!pool || !frames || !storage || count == 0 || frame_bytes == 0)
        return -1;

    memset(pool, 0, sizeof(*pool));
    INIT_LIST_HEAD(&pool->free);
    pool->frames = frames;
    pool->count = count;
    pool->frame_bytes = frame_bytes;

    for (u16 i = 0; i < count; i++) {
        frames[i].data = storage + (u32)i * frame_bytes;
        frames[i].bytes = frame_bytes;
        frames[i].frames = 0;
        frames[i].refs = 0;
        INIT_LIST_HEAD(&frames[i].node);
        list_add_tail(&frames[i].node, &pool->free);
    }
    return 0;
}

struct audio_frame *frame_pool_get(struct frame_pool *pool)
{
    if (!pool || list_empty(&pool->free))
        return NULL;

    struct audio_frame *frame = list_first_entry(&pool->free,
                                                  struct audio_frame, node);
    list_del_init(&frame->node);
    frame->refs = 1;
    frame->frames = 0;
    return frame;
}

void frame_pool_put(struct frame_pool *pool, struct audio_frame *frame)
{
    if (!pool || !frame || frame->refs == 0)
        return;
    if (--frame->refs)
        return;
    frame->frames = 0;
    list_add_tail(&frame->node, &pool->free);
}

u16 frame_pool_free(const struct frame_pool *pool)
{
    u16 count = 0;
    const struct list_head *pos;

    if (!pool)
        return 0;
    for (pos = pool->free.next; pos != &pool->free; pos = pos->next)
        count++;
    return count;
}

struct audio_frame *frame_pool_peek(struct frame_pool *pool)
{
    if (!pool || list_empty(&pool->free))
        return NULL;
    return list_first_entry(&pool->free, struct audio_frame, node);
}
