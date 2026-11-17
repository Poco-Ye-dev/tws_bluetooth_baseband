#ifndef __DRIVER_I2C_H__
#define __DRIVER_I2C_H__

#include <core/types.h>
#include <core/errno.h>

#define I2C_ADDR_7BIT(a) ((a) & 0x7F)

struct i2c_msg {
    u16 addr;
    u8 *buf;
    u16 len;
    bool read;
};

struct i2c_controller {
    const char *name;
    u32 clock_hz;
    int (*transfer)(struct i2c_controller *ctrl, struct i2c_msg *msgs, int num);
    int (*reset)(struct i2c_controller *ctrl);
    void *priv;
};

int i2c_register(struct i2c_controller *ctrl);
struct i2c_controller *i2c_get(const char *name);

int i2c_transfer(struct i2c_controller *ctrl, struct i2c_msg *msgs, int num);
int i2c_write_reg(struct i2c_controller *ctrl, u16 addr, u8 reg,
                  const void *data, u16 len);
int i2c_read_reg(struct i2c_controller *ctrl, u16 addr, u8 reg,
                 void *data, u16 len);
int i2c_write_reg8(struct i2c_controller *ctrl, u16 addr, u8 reg, u8 value);
int i2c_read_reg8(struct i2c_controller *ctrl, u16 addr, u8 reg, u8 *value);
int i2c_probe(struct i2c_controller *ctrl, u16 addr);

struct i2c_stats {
    u32 transfers;
    u32 errors;
    u32 retries;
    u32 recoveries;
};

int i2c_recover(struct i2c_controller *ctrl);
void i2c_get_stats(struct i2c_stats *stats);

#endif /* __DRIVER_I2C_H__ */
