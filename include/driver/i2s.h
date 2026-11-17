#ifndef __DRIVER_I2S_H__
#define __DRIVER_I2S_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Digital audio port.  One I2S instance carries playback and capture; the
 * audio path layer owns the ring buffers and hands the DMA addresses down.
 */

enum i2s_format {
    I2S_FMT_I2S = 0,
    I2S_FMT_LEFT_JUSTIFIED,
    I2S_FMT_RIGHT_JUSTIFIED,
    I2S_FMT_TDM,
};

struct i2s_config {
    u32 sample_rate;
    u8 channels;
    u8 bits;
    enum i2s_format format;
    bool master;
    bool capture;
};

struct i2s_port {
    const char *name;
    struct i2s_config config;
    bool running;
    void *priv;
};

struct i2s_ops {
    int (*configure)(struct i2s_port *port, const struct i2s_config *cfg);
    int (*start)(struct i2s_port *port);
    int (*stop)(struct i2s_port *port);
    int (*write)(struct i2s_port *port, const void *data, u32 frames);
    int (*read)(struct i2s_port *port, void *data, u32 frames);
};

int i2s_register(struct i2s_port *port, const struct i2s_ops *ops);
struct i2s_port *i2s_get(const char *name);
int i2s_configure(struct i2s_port *port, const struct i2s_config *cfg);
int i2s_start(struct i2s_port *port);
int i2s_stop(struct i2s_port *port);
int i2s_write(struct i2s_port *port, const void *data, u32 frames);
int i2s_read(struct i2s_port *port, void *data, u32 frames);

#endif /* __DRIVER_I2S_H__ */
