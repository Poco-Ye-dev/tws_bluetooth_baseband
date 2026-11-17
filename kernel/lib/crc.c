#include <core/crc.h>

static u32 crc32_table[256];
static u16 crc16_table[256];
static bool tables_ready;

static void build_tables(void)
{
    for (u32 i = 0; i < 256; i++) {
        u32 c = i;
        for (int bit = 0; bit < 8; bit++)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }

    for (u16 i = 0; i < 256; i++) {
        u16 c = i << 8;
        for (int bit = 0; bit < 8; bit++)
            c = (c & 0x8000) ? (u16)((c << 1) ^ 0x1021) : (u16)(c << 1);
        crc16_table[i] = c;
    }

    tables_ready = true;
}

u32 crc32_start(void)
{
    if (!tables_ready)
        build_tables();
    return 0xFFFFFFFFu;
}

u32 crc32_update(u32 crc, const void *data, size_t len)
{
    const u8 *p = data;

    if (!tables_ready)
        build_tables();

    while (len--)
        crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc;
}

u32 crc32_finish(u32 crc)
{
    return crc ^ 0xFFFFFFFFu;
}

u32 crc32(u32 seed, const void *data, size_t len)
{
    return crc32_finish(crc32_update(seed, data, len));
}

u16 crc16(u16 seed, const void *data, size_t len)
{
    const u8 *p = data;

    if (!tables_ready)
        build_tables();

    while (len--)
        seed = (u16)((seed << 8) ^ crc16_table[((seed >> 8) ^ *p++) & 0xFF]);
    return seed;
}

u8 checksum8(const void *data, size_t len)
{
    const u8 *p = data;
    u8 sum = 0;

    while (len--)
        sum = (u8)(sum + *p++);
    return sum;
}
