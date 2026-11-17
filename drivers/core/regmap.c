#include <driver/regmap.h>
#include <core/string.h>
#include <core/errno.h>

static volatile u8 *at(struct regmap *map, u32 reg)
{
    return (volatile u8 *)map->base + (size_t)reg * map->stride;
}

int regmap_init_mmio(struct regmap *map, const char *name, void *base,
                     u8 reg_bits, u8 val_bits)
{
    if (!map || !base)
        return -EINVAL;
    if (reg_bits != 8 && reg_bits != 16 && reg_bits != 32)
        return -EINVAL;
    if (val_bits != 8 && val_bits != 16 && val_bits != 32)
        return -EINVAL;

    memset(map, 0, sizeof(*map));
    map->base = base;
    map->stride = 1;
    map->reg_bits = reg_bits;
    map->val_bits = val_bits;
    map->name = name;
    return 0;
}

static u32 raw_read(struct regmap *map, u32 reg)
{
    volatile u8 *p = at(map, reg);

    switch (map->val_bits) {
    case 8:
        return *(volatile u8 *)p;
    case 16: {
        u16 v = *(volatile u16 *)p;
        return map->big_endian ? swap16(v) : v;
    }
    default: {
        u32 v = *(volatile u32 *)p;
        return map->big_endian ? swap32(v) : v;
    }
    }
}

static void raw_write(struct regmap *map, u32 reg, u32 value)
{
    volatile u8 *p = at(map, reg);

    switch (map->val_bits) {
    case 8:
        *(volatile u8 *)p = (u8)value;
        break;
    case 16:
        *(volatile u16 *)p = map->big_endian ? swap16((u16)value) : (u16)value;
        break;
    default:
        *(volatile u32 *)p = map->big_endian ? swap32(value) : value;
        break;
    }
}

u32 regmap_read(struct regmap *map, u32 reg)
{
    struct irq_flags flags;
    u32 value;

    spin_lock_irqsave(&map->lock, flags);
    value = raw_read(map, reg);
    spin_unlock_irqrestore(&map->lock, flags);
    return value;
}

int regmap_write(struct regmap *map, u32 reg, u32 value)
{
    struct irq_flags flags;

    if (!map)
        return -EINVAL;

    spin_lock_irqsave(&map->lock, flags);
    raw_write(map, reg, value);
    spin_unlock_irqrestore(&map->lock, flags);
    return 0;
}

int regmap_update_bits(struct regmap *map, u32 reg, u32 mask, u32 value)
{
    struct irq_flags flags;
    u32 old;

    if (!map)
        return -EINVAL;
    if (value & ~mask)
        return -EINVAL;

    spin_lock_irqsave(&map->lock, flags);
    old = raw_read(map, reg);
    raw_write(map, reg, (old & ~mask) | (value & mask));
    spin_unlock_irqrestore(&map->lock, flags);
    return 0;
}

bool regmap_test_bits(struct regmap *map, u32 reg, u32 mask)
{
    return (regmap_read(map, reg) & mask) != 0;
}

int regmap_write_bulk(struct regmap *map, u32 reg, const void *data, u32 count)
{
    const u8 *src = data;

    if (!map || !data)
        return -EINVAL;

    for (u32 i = 0; i < count; i++)
        raw_write(map, reg + i, src[i]);
    return 0;
}

int regmap_read_bulk(struct regmap *map, u32 reg, void *data, u32 count)
{
    u8 *dst = data;

    if (!map || !data)
        return -EINVAL;

    for (u32 i = 0; i < count; i++)
        dst[i] = (u8)raw_read(map, reg + i);
    return 0;
}
