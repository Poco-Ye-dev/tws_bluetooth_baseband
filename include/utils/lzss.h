#ifndef __UTILS_LZSS_H__
#define __UTILS_LZSS_H__

#include <core/types.h>

/*
 * LZSS compressor for firmware resources and log snapshots.  The format uses
 * 8 byte flag groups, 12 bit offsets and 4 bit lengths (3..18), which is a
 * good trade-off between ratio and decoder footprint on a small core.
 */

#define LZSS_WINDOW 4096
#define LZSS_MIN_MATCH 3
#define LZSS_MAX_MATCH 18

u32 lzss_compress(const u8 *in, u32 in_len, u8 *out, u32 out_cap);
int lzss_decompress(const u8 *in, u32 in_len, u8 *out, u32 out_cap);

#endif /* __UTILS_LZSS_H__ */
