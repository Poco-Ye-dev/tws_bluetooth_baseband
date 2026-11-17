#include <bt/frame.h>
#include <core/string.h>

static struct ctrl_frame frames[EDR_FRAME_MAX];
static struct ctrl_frame *queue_head;

void edr_frame_init(void)
{
    memset(frames, 0, sizeof(frames));
    queue_head = NULL;
}

struct ctrl_frame *edr_frame_alloc(u16 handle, u8 lt_addr)
{
    for (unsigned int i = 0; i < EDR_FRAME_MAX; i++) {
        if (frames[i].used)
            continue;
        memset(&frames[i], 0, sizeof(frames[i]));
        frames[i].used = true;
        frames[i].handle = handle;
        frames[i].lt_addr = lt_addr;
        return &frames[i];
    }
    return NULL;
}

void edr_frame_free(struct ctrl_frame *frame)
{
    if (frame)
        memset(frame, 0, sizeof(*frame));
}

int edr_frame_enqueue(struct ctrl_frame *frame, u8 prio)
{
    struct ctrl_frame **at = &queue_head;

    if (!frame)
        return -1;
    frame->prio = prio;
    frame->retries = 0;

    while (*at && (*at)->prio >= prio)
        at = &(*at)->next;

    frame->next = *at;
    *at = frame;
    return 0;
}

struct ctrl_frame *edr_frame_peek(u16 handle)
{
    for (struct ctrl_frame *f = queue_head; f; f = f->next) {
        if (f->handle == handle)
            return f;
    }
    return NULL;
}

struct ctrl_frame *edr_frame_dequeue(u16 handle)
{
    struct ctrl_frame **at = &queue_head;

    while (*at) {
        if ((*at)->handle == handle) {
            struct ctrl_frame *f = *at;
            *at = f->next;
            f->next = NULL;
            return f;
        }
        at = &(*at)->next;
    }
    return NULL;
}

void edr_frame_ack(u16 handle, u8 arqn)
{
    struct ctrl_frame **at = &queue_head;

    while (*at) {
        if ((*at)->handle == handle && ((*at)->seqn & 1) == (arqn & 1)) {
            struct ctrl_frame *f = *at;
            *at = f->next;
            edr_frame_free(f);
            return;
        }
        at = &(*at)->next;
    }
}

void edr_frame_retry(struct ctrl_frame *frame)
{
    if (frame)
        frame->retries++;
}

void edr_frame_flush(u16 handle)
{
    struct ctrl_frame **at = &queue_head;

    while (*at) {
        if ((*at)->handle == handle) {
            struct ctrl_frame *f = *at;
            *at = f->next;
            edr_frame_free(f);
        } else {
            at = &(*at)->next;
        }
    }
}

u8 edr_frame_pending(u16 handle)
{
    u8 count = 0;
    for (struct ctrl_frame *f = queue_head; f; f = f->next) {
        if (f->handle == handle)
            count++;
    }
    return count;
}

/*
 * AFH remap: translate the logical frequency table to physical channels,
 * skipping channels absent from the map.  This mirrors bdhw_set_afh().
 */
int edr_frame_afh_remap(const u8 *frq_table, const u8 *channel_map,
                        u8 *out_map)
{
    u8 used = 0;
    u8 out = 0;

    if (!frq_table || !channel_map || !out_map)
        return -1;

    for (u16 ch = 0; ch < 79; ch++) {
        if ((channel_map[ch >> 3] >> (ch & 7)) & 1) {
            out_map[out++] = frq_table[ch];
            used++;
        }
    }
    return used;
}
