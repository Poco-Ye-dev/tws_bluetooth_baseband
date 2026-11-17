#include <update/ota.h>
#include <driver/flash.h>
#include <core/crc.h>
#include <core/string.h>
#include <core/printk.h>

#define OTA_STAGING_PARTITION "ota"

static struct {
    struct ota_status status;
    struct ota_header header;
    u32 running_crc;
} ota;

int ota_verify_header(const struct ota_header *header)
{
    if (!header || header->magic != OTA_MAGIC)
        return -EINVAL;

    u32 crc = crc32(0, (const u8 *)header,
                    offsetof(struct ota_header, header_crc));
    if (crc != header->header_crc)
        return -EPROTO;

    if (header->image_size == 0 || header->header_size < sizeof(*header))
        return -EINVAL;

    return 0;
}

int ota_begin(const struct ota_header *header)
{
    int ret;

    if (ota.status.state == OTA_STATE_RECEIVING)
        return -EBUSY;

    ret = ota_verify_header(header);
    if (ret) {
        ota.status.state = OTA_STATE_FAILED;
        return ret;
    }

    ota.header = *header;
    ota.running_crc = crc32_start();
    ota.status.state = OTA_STATE_RECEIVING;
    ota.status.received = 0;
    ota.status.total = header->image_size;
    ota.status.image_crc = 0;

    ret = flash_partition_erase(OTA_STAGING_PARTITION, 0,
                                (header->image_size + 4095u) & ~4095u);
    if (ret && ret != -ENOENT)
        return ret;

    pr_info("ota: receiving %u bytes, version %u.%u\n",
            header->image_size, header->version_major, header->version_minor);
    return 0;
}

int ota_write(const void *data, u32 len)
{
    int ret;

    if (ota.status.state != OTA_STATE_RECEIVING)
        return -EPERM;

    if (len > ota.status.total - ota.status.received)
        return -ERANGE;

    ret = flash_partition_write(OTA_STAGING_PARTITION, ota.status.received,
                                data, len);
    if (ret && ret != -ENOENT)
        return ret;

    ota.running_crc = crc32_update(ota.running_crc, data, len);
    ota.status.received += len;
    return 0;
}

int ota_finish(void)
{
    if (ota.status.state != OTA_STATE_RECEIVING)
        return -EPERM;

    ota.status.image_crc = crc32_finish(ota.running_crc);
    ota.status.state = OTA_STATE_VERIFYING;

    if (ota.status.received != ota.status.total ||
        ota.status.image_crc != ota.header.image_crc) {
        ota.status.state = OTA_STATE_FAILED;
        pr_err("ota: image check failed\n");
        return -EPROTO;
    }

    ota.status.state = OTA_STATE_READY;
    pr_info("ota: image ready\n");
    return 0;
}

void ota_abort(void)
{
    ota.status.state = OTA_STATE_IDLE;
    ota.status.received = 0;
}

const struct ota_status *ota_get_status(void)
{
    return &ota.status;
}
