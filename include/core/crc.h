#ifndef __CORE_CRC_H__
#define __CORE_CRC_H__

#include <core/types.h>

/*
 * Checksums used by the OTA image format and by the configuration store.
 *
 * crc32() is the reflected IEEE 802.3 polynomial (0xEDB88320); crc16() is
 * the CCITT polynomial used by the flash translation layer.
 */

u32 crc32(u32 seed, const void *data, size_t len);
u16 crc16(u16 seed, const void *data, size_t len);
u8  checksum8(const void *data, size_t len);

/* Incremental interface for streaming updates. */
u32 crc32_start(void);
u32 crc32_update(u32 crc, const void *data, size_t len);
u32 crc32_finish(u32 crc);

#endif /* __CORE_CRC_H__ */
