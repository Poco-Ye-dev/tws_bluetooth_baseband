#include <utils/lzss.h>
#include <core/string.h>

/*
 * Greedy LZSS matching.  A full hash chain is overkill for the resource
 * sizes involved; a bounded backwards scan finds most matches within a
 * predictable cycle budget.
 */

u32 lzss_compress(const u8 *in, u32 in_len, u8 *out, u32 out_cap)
{
    u32 in_pos = 0;
    u32 out_pos = 0;

    if (!in || !out)
        return 0;

    while (in_pos < in_len) {
        u32 flag_pos = out_pos++;
        u8 flags = 0;

        if (flag_pos >= out_cap)
            return 0;

        for (int bit = 0; bit < 8 && in_pos < in_len; bit++) {
            u32 best_len = 0;
            u32 best_off = 0;
            u32 window = in_pos < LZSS_WINDOW ? in_pos : LZSS_WINDOW;

            for (u32 off = 1; off <= window; off++) {
                u32 len = 0;
                while (len < LZSS_MAX_MATCH &&
                       in_pos + len < in_len &&
                       in[in_pos + len] == in[in_pos - off + len])
                    len++;

                if (len > best_len) {
                    best_len = len;
                    best_off = off;
                    if (len == LZSS_MAX_MATCH)
                        break;
                }
            }

            if (best_len >= LZSS_MIN_MATCH && out_pos + 2 < out_cap) {
                u16 value = (u16)(((best_off - 1) & 0x0FFF) |
                                  ((best_len - LZSS_MIN_MATCH) << 12));
                out[out_pos++] = (u8)(value & 0xFF);
                out[out_pos++] = (u8)(value >> 8);
                flags |= (u8)(1u << bit);
                in_pos += best_len;
            } else {
                if (out_pos >= out_cap)
                    return 0;
                out[out_pos++] = in[in_pos++];
            }
        }

        out[flag_pos] = flags;
    }

    return out_pos;
}

int lzss_decompress(const u8 *in, u32 in_len, u8 *out, u32 out_cap)
{
    u32 in_pos = 0;
    u32 out_pos = 0;

    if (!in || !out)
        return -1;

    while (in_pos < in_len) {
        u8 flags = in[in_pos++];

        for (int bit = 0; bit < 8 && in_pos < in_len; bit++) {
            if (flags & (1u << bit)) {
                if (in_pos + 1 >= in_len)
                    return -1;
                u16 value = (u16)(in[in_pos] | (in[in_pos + 1] << 8));
                in_pos += 2;

                u32 off = (u32)(value & 0x0FFF) + 1;
                u32 len = (u32)((value >> 12) & 0x0F) + LZSS_MIN_MATCH;

                if (off > out_pos)
                    return -1;
                if (out_pos + len > out_cap)
                    return -1;

                for (u32 i = 0; i < len; i++) {
                    out[out_pos] = out[out_pos - off];
                    out_pos++;
                }
            } else {
                if (out_pos >= out_cap)
                    return -1;
                out[out_pos++] = in[in_pos];
                in_pos++;
            }
        }
    }

    return (int)out_pos;
}
