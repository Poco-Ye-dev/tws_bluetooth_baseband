#include <driver/dma.h>
#include <core/string.h>

#define DMA_MAX_CONTROLLERS 3
#define DMA_MAX_CHANNELS 12

static struct dma_controller *controllers[DMA_MAX_CONTROLLERS];
static unsigned int controller_count;

static struct dma_slot {
    struct dma_channel channel;
    struct dma_controller *ctrl;
    bool used;
} slots[DMA_MAX_CHANNELS];

int dma_register(struct dma_controller *ctrl)
{
    if (!ctrl || !ctrl->start || controller_count >= DMA_MAX_CONTROLLERS)
        return -EINVAL;
    controllers[controller_count++] = ctrl;
    return 0;
}

struct dma_controller *dma_get(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

struct dma_channel *dma_request(struct dma_controller *ctrl,
                                enum dma_direction dir, bool cyclic)
{
    if (!ctrl)
        return NULL;

    for (unsigned int i = 0; i < DMA_MAX_CHANNELS; i++) {
        if (slots[i].used)
            continue;

        slots[i].used = true;
        slots[i].ctrl = ctrl;
        slots[i].channel.id = (u8)i;
        slots[i].channel.dir = dir;
        slots[i].channel.cyclic = cyclic;
        slots[i].channel.busy = false;

        if (ctrl->request && ctrl->request(ctrl, &slots[i].channel)) {
            slots[i].used = false;
            return NULL;
        }
        return &slots[i].channel;
    }
    return NULL;
}

void dma_release(struct dma_channel *ch)
{
    if (!ch)
        return;

    for (unsigned int i = 0; i < DMA_MAX_CHANNELS; i++) {
        if (&slots[i].channel == ch) {
            if (slots[i].ctrl->stop)
                slots[i].ctrl->stop(slots[i].ctrl, ch);
            slots[i].used = false;
            return;
        }
    }
}

int dma_start(struct dma_channel *ch, const void *src, void *dst, u32 len,
              dma_cb_t cb, void *arg)
{
    struct dma_slot *slot = NULL;

    if (!ch || ch->busy)
        return -EBUSY;

    for (unsigned int i = 0; i < DMA_MAX_CHANNELS; i++) {
        if (&slots[i].channel == ch) {
            slot = &slots[i];
            break;
        }
    }
    if (!slot)
        return -EINVAL;

    ch->busy = true;
    int ret = slot->ctrl->start(slot->ctrl, ch, src, dst, len, cb, arg);
    if (ret)
        ch->busy = false;
    return ret;
}

int dma_stop(struct dma_channel *ch)
{
    for (unsigned int i = 0; i < DMA_MAX_CHANNELS; i++) {
        if (&slots[i].channel == ch) {
            int ret = 0;
            if (slots[i].ctrl->stop)
                ret = slots[i].ctrl->stop(slots[i].ctrl, ch);
            ch->busy = false;
            return ret;
        }
    }
    return -EINVAL;
}

bool dma_busy(const struct dma_channel *ch)
{
    return ch && ch->busy;
}
