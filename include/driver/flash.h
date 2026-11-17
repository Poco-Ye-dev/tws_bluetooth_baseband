#ifndef __DRIVER_FLASH_H__
#define __DRIVER_FLASH_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * NOR/NAND flash abstraction with a simple partition table.  The OTA and
 * configuration layers only ever address partitions, never raw offsets.
 */

#define FLASH_NAME_MAX 16

struct flash_partition {
    char name[FLASH_NAME_MAX];
    u32 offset;
    u32 size;
};

struct flash_info {
    u32 total_size;
    u32 erase_size;
    u32 write_size;
};

struct flash_device {
    const char *name;
    struct flash_info info;
    int (*read)(struct flash_device *dev, u32 offset, void *buf, u32 len);
    int (*write)(struct flash_device *dev, u32 offset, const void *buf, u32 len);
    int (*erase)(struct flash_device *dev, u32 offset, u32 len);
    void *priv;
};

int flash_register(struct flash_device *dev);
struct flash_device *flash_get(const char *name);

int flash_read(struct flash_device *dev, u32 offset, void *buf, u32 len);
int flash_write(struct flash_device *dev, u32 offset, const void *buf, u32 len);
int flash_erase(struct flash_device *dev, u32 offset, u32 len);

int flash_add_partitions(const struct flash_partition *table, unsigned int count);
const struct flash_partition *flash_find_partition(const char *name);
int flash_partition_read(const char *name, u32 offset, void *buf, u32 len);
int flash_partition_write(const char *name, u32 offset, const void *buf, u32 len);
int flash_partition_erase(const char *name, u32 offset, u32 len);

/* Crash-safe two-slot configuration store (sequence + CRC). */
int flash_config_save(const char *partition, const void *data, u16 len);
int flash_config_load(const char *partition, void *data, u16 *len);

#endif /* __DRIVER_FLASH_H__ */
