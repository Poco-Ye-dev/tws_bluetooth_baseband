#ifndef __DRIVER_REGMAP_H__
#define __DRIVER_REGMAP_H__

#include <core/types.h>
#include <core/spinlock.h>

/*
 * Register map abstraction.
 *
 * Drivers describe a block once (base, stride, access width) and then read or
 * modify fields without repeating volatile casts.  Bitfield updates are
 * read-modify-write under a lock, which is what multi-driver peripherals
 * require.
 */

struct regmap {
    void *base;
    u16 stride;
    u8 reg_bits;
    u8 val_bits;
    bool big_endian;
    spinlock_t lock;
    const char *name;
};

int regmap_init_mmio(struct regmap *map, const char *name, void *base,
                     u8 reg_bits, u8 val_bits);
u32 regmap_read(struct regmap *map, u32 reg);
int regmap_write(struct regmap *map, u32 reg, u32 value);
int regmap_update_bits(struct regmap *map, u32 reg, u32 mask, u32 value);
int regmap_write_bulk(struct regmap *map, u32 reg, const void *data, u32 count);
int regmap_read_bulk(struct regmap *map, u32 reg, void *data, u32 count);
bool regmap_test_bits(struct regmap *map, u32 reg, u32 mask);

#endif /* __DRIVER_REGMAP_H__ */
