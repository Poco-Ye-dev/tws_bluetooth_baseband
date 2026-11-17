#include <driver/flash.h>
#include <core/string.h>
#include <core/crc.h>

#define FLASH_MAX_PARTITIONS 16

static struct flash_device *device;
static struct flash_partition partitions[FLASH_MAX_PARTITIONS];
static unsigned int partition_count;

int flash_register(struct flash_device *dev)
{
    if (!dev || !dev->read || !dev->write || !dev->erase)
        return -EINVAL;
    device = dev;
    return 0;
}

struct flash_device *flash_get(const char *name)
{
    if (device && strcmp(device->name, name) == 0)
        return device;
    return NULL;
}

static int check_range(struct flash_device *dev, u32 offset, u32 len)
{
    if (!dev)
        return -ENODEV;
    if (offset > dev->info.total_size ||
        len > dev->info.total_size - offset)
        return -ERANGE;
    return 0;
}

int flash_read(struct flash_device *dev, u32 offset, void *buf, u32 len)
{
    int ret = check_range(dev, offset, len);
    return ret ? ret : dev->read(dev, offset, buf, len);
}

int flash_write(struct flash_device *dev, u32 offset, const void *buf, u32 len)
{
    int ret = check_range(dev, offset, len);
    return ret ? ret : dev->write(dev, offset, buf, len);
}

int flash_erase(struct flash_device *dev, u32 offset, u32 len)
{
    int ret = check_range(dev, offset, len);
    return ret ? ret : dev->erase(dev, offset, len);
}

int flash_add_partitions(const struct flash_partition *table, unsigned int count)
{
    if (!table || partition_count + count > FLASH_MAX_PARTITIONS)
        return -ENOSPC;

    memcpy(&partitions[partition_count], table,
           count * sizeof(struct flash_partition));
    partition_count += count;
    return 0;
}

const struct flash_partition *flash_find_partition(const char *name)
{
    for (unsigned int i = 0; i < partition_count; i++) {
        if (strcmp(partitions[i].name, name) == 0)
            return &partitions[i];
    }
    return NULL;
}

int flash_partition_read(const char *name, u32 offset, void *buf, u32 len)
{
    const struct flash_partition *p = flash_find_partition(name);

    if (!p)
        return -ENOENT;
    if (offset > p->size || len > p->size - offset)
        return -ERANGE;
    return flash_read(device, p->offset + offset, buf, len);
}

int flash_partition_write(const char *name, u32 offset, const void *buf, u32 len)
{
    const struct flash_partition *p = flash_find_partition(name);

    if (!p)
        return -ENOENT;
    if (offset > p->size || len > p->size - offset)
        return -ERANGE;
    return flash_write(device, p->offset + offset, buf, len);
}

int flash_partition_erase(const char *name, u32 offset, u32 len)
{
    const struct flash_partition *p = flash_find_partition(name);

    if (!p)
        return -ENOENT;
    if (offset > p->size || len > p->size - offset)
        return -ERANGE;
    return flash_erase(device, p->offset + offset, len);
}

#define CFG_MAGIC 0x47464354u

struct cfg_slot {
    u32 magic;
    u32 sequence;
    u32 crc;
    u16 len;
    u8 data[128];
};

static bool slot_valid(const struct cfg_slot *slot)
{
    if (slot->magic != CFG_MAGIC)
        return false;
    if (slot->len > sizeof(slot->data))
        return false;
    return crc32(0, (const u8 *)slot,
                 offsetof(struct cfg_slot, crc) + sizeof(slot->crc)) == slot->crc;
}

int flash_config_save(const char *partition, const void *data, u16 len)
{
    struct cfg_slot slots[2];
    struct cfg_slot next;
    int ret;

    if (!data || len > sizeof(next.data))
        return -EINVAL;

    memset(slots, 0, sizeof(slots));
    flash_partition_read(partition, 0, &slots[0], sizeof(slots[0]));
    flash_partition_read(partition, sizeof(slots[0]), &slots[1], sizeof(slots[1]));

    u32 sequence = 1;
    for (int i = 0; i < 2; i++) {
        if (slot_valid(&slots[i]) && slots[i].sequence >= sequence)
            sequence = slots[i].sequence + 1;
    }

    memset(&next, 0, sizeof(next));
    next.magic = CFG_MAGIC;
    next.sequence = sequence;
    next.len = len;
    memcpy(next.data, data, len);
    next.crc = crc32(0, (const u8 *)&next,
                     offsetof(struct cfg_slot, crc) + sizeof(next.crc));

    unsigned int target = (sequence & 1) ? 0 : 1;
    u32 offset = target * sizeof(struct cfg_slot);

    ret = flash_partition_erase(partition, offset, 4096);
    if (ret && ret != -ENOENT)
        return ret;

    return flash_partition_write(partition, offset, &next, sizeof(next));
}

int flash_config_load(const char *partition, void *data, u16 *len)
{
    struct cfg_slot slots[2];
    struct cfg_slot *best = NULL;

    if (!data || !len)
        return -EINVAL;

    memset(slots, 0, sizeof(slots));
    flash_partition_read(partition, 0, &slots[0], sizeof(slots[0]));
    flash_partition_read(partition, sizeof(slots[0]), &slots[1], sizeof(slots[1]));

    for (int i = 0; i < 2; i++) {
        if (!slot_valid(&slots[i]))
            continue;
        if (!best || slots[i].sequence > best->sequence)
            best = &slots[i];
    }

    if (!best)
        return -ENOENT;

    memcpy(data, best->data, best->len);
    *len = best->len;
    return 0;
}
