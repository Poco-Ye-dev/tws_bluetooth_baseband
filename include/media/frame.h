#ifndef __MEDIA_FRAME_H__
#define __MEDIA_FRAME_H__

#include <core/types.h>
#include <core/list.h>

/*
 * Fixed size audio frame pool.
 *
 * Audio paths exchange frame descriptors instead of raw pointers so the
 * producer and consumer can agree on a frame's lifetime without copying.
 * The pool storage is supplied by the board (usually a static array) and the
 * core only maintains the free list.
 */

struct audio_frame {
    struct list_head node;
    u8 *data;
    u16 bytes;
    u16 frames;
    u8 channels;
    u8 bits;
    u16 refs;
};

struct frame_pool {
    struct audio_frame *frames;
    u16 count;
    u16 frame_bytes;
    struct list_head free;
};

int frame_pool_init(struct frame_pool *pool, struct audio_frame *frames,
                    u8 *storage, u16 count, u16 frame_bytes);
struct audio_frame *frame_pool_get(struct frame_pool *pool);
void frame_pool_put(struct frame_pool *pool, struct audio_frame *frame);
u16 frame_pool_free(const struct frame_pool *pool);
struct audio_frame *frame_pool_peek(struct frame_pool *pool);

#endif /* __MEDIA_FRAME_H__ */
