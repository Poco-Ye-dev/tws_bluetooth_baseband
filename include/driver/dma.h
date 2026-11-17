#ifndef __DRIVER_DMA_H__
#define __DRIVER_DMA_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Descriptor based DMA.  Audio uses cyclic channels; flash and UART use
 * one-shot transfers.  Completion is reported from interrupt context.
 */

enum dma_direction {
    DMA_MEM_TO_MEM = 0,
    DMA_MEM_TO_DEV,
    DMA_DEV_TO_MEM,
};

typedef void (*dma_cb_t)(void *arg);

struct dma_channel {
    u8 id;
    enum dma_direction dir;
    bool cyclic;
    bool busy;
    void *priv;
};

struct dma_controller {
    const char *name;
    unsigned int nchannels;
    int (*request)(struct dma_controller *ctrl, struct dma_channel *ch);
    int (*start)(struct dma_controller *ctrl, struct dma_channel *ch,
                 const void *src, void *dst, u32 len, dma_cb_t cb, void *arg);
    int (*stop)(struct dma_controller *ctrl, struct dma_channel *ch);
    void *priv;
};

int dma_register(struct dma_controller *ctrl);
struct dma_controller *dma_get(const char *name);

struct dma_channel *dma_request(struct dma_controller *ctrl,
                                enum dma_direction dir, bool cyclic);
void dma_release(struct dma_channel *ch);
int dma_start(struct dma_channel *ch, const void *src, void *dst, u32 len,
              dma_cb_t cb, void *arg);
int dma_stop(struct dma_channel *ch);
bool dma_busy(const struct dma_channel *ch);

#endif /* __DRIVER_DMA_H__ */
