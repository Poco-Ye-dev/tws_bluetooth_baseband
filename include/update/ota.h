#ifndef __UPDATE_OTA_H__
#define __UPDATE_OTA_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Over-the-air / local firmware update.
 *
 * An image starts with ota_header, is written into a staging partition and is
 * only made bootable after the CRC and signature checks pass.  A power cut at
 * any point leaves the running image untouched.
 */

#define OTA_MAGIC 0x41544F54u   /* "TOTA" */
#define OTA_VERSION_MAX 24

enum ota_state {
    OTA_STATE_IDLE = 0,
    OTA_STATE_RECEIVING,
    OTA_STATE_VERIFYING,
    OTA_STATE_READY,
    OTA_STATE_FAILED,
};

struct ota_header {
    u32 magic;
    u32 header_size;
    u32 image_size;
    u32 image_crc;
    u32 header_crc;
    u16 version_major;
    u16 version_minor;
    u8 timestamp[8];
    char name[OTA_VERSION_MAX];
};

struct ota_status {
    enum ota_state state;
    u32 received;
    u32 total;
    u32 image_crc;
};

int ota_begin(const struct ota_header *header);
int ota_write(const void *data, u32 len);
int ota_finish(void);
void ota_abort(void);
const struct ota_status *ota_get_status(void);
int ota_verify_header(const struct ota_header *header);

#endif /* __UPDATE_OTA_H__ */
