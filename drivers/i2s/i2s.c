#include <driver/i2s.h>
#include <core/string.h>

#define I2S_MAX_PORTS 2

static struct {
    struct i2s_port *port;
    const struct i2s_ops *ops;
} ports[I2S_MAX_PORTS];
static unsigned int port_count;

int i2s_register(struct i2s_port *port, const struct i2s_ops *ops)
{
    if (!port || !ops || port_count >= I2S_MAX_PORTS)
        return -EINVAL;

    ports[port_count].port = port;
    ports[port_count].ops = ops;
    port_count++;
    return 0;
}

struct i2s_port *i2s_get(const char *name)
{
    for (unsigned int i = 0; i < port_count; i++) {
        if (strcmp(ports[i].port->name, name) == 0)
            return ports[i].port;
    }
    return NULL;
}

static const struct i2s_ops *ops_for(struct i2s_port *port)
{
    for (unsigned int i = 0; i < port_count; i++) {
        if (ports[i].port == port)
            return ports[i].ops;
    }
    return NULL;
}

int i2s_configure(struct i2s_port *port, const struct i2s_config *cfg)
{
    const struct i2s_ops *ops;

    if (!port || !cfg || !cfg->sample_rate || !cfg->channels)
        return -EINVAL;

    ops = ops_for(port);
    if (!ops || !ops->configure)
        return -ENOSYS;

    int ret = ops->configure(port, cfg);
    if (!ret)
        port->config = *cfg;
    return ret;
}

int i2s_start(struct i2s_port *port)
{
    const struct i2s_ops *ops = ops_for(port);

    if (!ops || !ops->start)
        return -ENOSYS;
    if (port->running)
        return -EBUSY;

    int ret = ops->start(port);
    if (!ret)
        port->running = true;
    return ret;
}

int i2s_stop(struct i2s_port *port)
{
    const struct i2s_ops *ops = ops_for(port);

    if (!ops || !ops->stop)
        return -ENOSYS;
    if (!port->running)
        return 0;

    int ret = ops->stop(port);
    port->running = false;
    return ret;
}

int i2s_write(struct i2s_port *port, const void *data, u32 frames)
{
    const struct i2s_ops *ops = ops_for(port);

    if (!ops || !ops->write)
        return -ENOSYS;
    if (!port->running)
        return -EPERM;
    return ops->write(port, data, frames);
}

int i2s_read(struct i2s_port *port, void *data, u32 frames)
{
    const struct i2s_ops *ops = ops_for(port);

    if (!ops || !ops->read)
        return -ENOSYS;
    if (!port->running)
        return -EPERM;
    return ops->read(port, data, frames);
}
