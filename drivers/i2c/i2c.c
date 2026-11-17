#include <driver/i2c.h>
#include <core/string.h>

#define I2C_MAX_CONTROLLERS 3
#define I2C_MAX_RETRIES 3

static struct i2c_controller *controllers[I2C_MAX_CONTROLLERS];
static unsigned int controller_count;
static struct i2c_stats stats;

int i2c_register(struct i2c_controller *ctrl)
{
    if (!ctrl || !ctrl->transfer || controller_count >= I2C_MAX_CONTROLLERS)
        return -EINVAL;

    controllers[controller_count++] = ctrl;
    return 0;
}

struct i2c_controller *i2c_get(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

int i2c_recover(struct i2c_controller *ctrl)
{
    if (!ctrl || !ctrl->reset)
        return -ENOSYS;
    stats.recoveries++;
    return ctrl->reset(ctrl);
}

int i2c_transfer(struct i2c_controller *ctrl, struct i2c_msg *msgs, int num)
{
    int ret;

    if (!ctrl || !ctrl->transfer || !msgs || num <= 0)
        return -EINVAL;

    stats.transfers++;

    ret = ctrl->transfer(ctrl, msgs, num);
    if (!ret)
        return 0;

    for (int attempt = 0; attempt < I2C_MAX_RETRIES; attempt++) {
        stats.retries++;
        ret = ctrl->transfer(ctrl, msgs, num);
        if (!ret)
            return 0;
    }

    stats.errors++;
    i2c_recover(ctrl);
    return ret;
}

void i2c_get_stats(struct i2c_stats *out)
{
    if (out)
        *out = stats;
}

int i2c_write_reg(struct i2c_controller *ctrl, u16 addr, u8 reg,
                  const void *data, u16 len)
{
    u8 buf[64];
    struct i2c_msg msg;

    if ((size_t)len + 1 > sizeof(buf))
        return -EINVAL;

    buf[0] = reg;
    if (len)
        memcpy(&buf[1], data, len);

    msg.addr = addr;
    msg.buf = buf;
    msg.len = len + 1;
    msg.read = false;
    return i2c_transfer(ctrl, &msg, 1);
}

int i2c_read_reg(struct i2c_controller *ctrl, u16 addr, u8 reg,
                 void *data, u16 len)
{
    struct i2c_msg msgs[2];

    msgs[0].addr = addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].read = false;

    msgs[1].addr = addr;
    msgs[1].buf = data;
    msgs[1].len = len;
    msgs[1].read = true;

    return i2c_transfer(ctrl, msgs, 2);
}

int i2c_write_reg8(struct i2c_controller *ctrl, u16 addr, u8 reg, u8 value)
{
    return i2c_write_reg(ctrl, addr, reg, &value, 1);
}

int i2c_read_reg8(struct i2c_controller *ctrl, u16 addr, u8 reg, u8 *value)
{
    return i2c_read_reg(ctrl, addr, reg, value, 1);
}

int i2c_probe(struct i2c_controller *ctrl, u16 addr)
{
    struct i2c_msg msg;
    u8 dummy = 0;

    msg.addr = addr;
    msg.buf = &dummy;
    msg.len = 1;
    msg.read = true;
    return i2c_transfer(ctrl, &msg, 1);
}
