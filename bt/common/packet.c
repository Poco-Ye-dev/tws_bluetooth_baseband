#include <bt/packet.h>
#include <core/string.h>

/*
 * Bit oriented baseband coding.
 *
 * The air format is little endian at the bit level, so all helpers append or
 * consume bits starting with the least significant bit of each byte.
 */

static void put_bits(u8 *out, u32 *pos, u32 value, u32 count)
{
    for (u32 i = 0; i < count; i++) {
        u32 bit = (value >> i) & 1;
        u32 p = *pos + i;
        if (bit)
            out[p >> 3] |= (u8)(1u << (p & 7));
        else
            out[p >> 3] &= (u8)~(1u << (p & 7));
    }
    *pos += count;
}

static u32 get_bits(const u8 *in, u32 *pos, u32 count)
{
    u32 value = 0;

    for (u32 i = 0; i < count; i++) {
        u32 p = *pos + i;
        value |= (u32)((in[p >> 3] >> (p & 7)) & 1) << i;
    }
    *pos += count;
    return value;
}

u8 bb_hec(u8 header10, const bt_addr *master)
{
    u8 hec = master ? master->b[0] : 0;

    for (int i = 0; i < 10; i++) {
        u8 feedback = (u8)(((hec >> 7) & 1) ^ ((header10 >> i) & 1));
        hec = (u8)(hec << 1);
        if (feedback)
            hec ^= 0x35;
    }
    return hec;
}

void bb_header_encode(const struct bb_header *hdr, u8 *out, u32 *bits)
{
    u32 pos = 0;
    u8 header10 = (u8)((hdr->lt_addr & 0x07) |
                       ((hdr->type & 0x0F) << 3) |
                       ((hdr->flow & 0x01) << 7) |
                       ((hdr->arqn & 0x01) << 8) |
                       ((hdr->seqn & 0x01) << 9));

    memset(out, 0, 4);
    put_bits(out, &pos, header10, 10);
    put_bits(out, &pos, 0, 0);
    if (bits)
        *bits = pos;
}

int bb_header_decode(const u8 *in, u32 bits, struct bb_header *hdr)
{
    u32 pos = 0;
    (void)bits;

    u8 header10 = (u8)get_bits(in, &pos, 10);

    hdr->lt_addr = header10 & 0x07;
    hdr->type = (header10 >> 3) & 0x0F;
    hdr->flow = (header10 >> 7) & 0x01;
    hdr->arqn = (header10 >> 8) & 0x01;
    hdr->seqn = (header10 >> 9) & 0x01;
    return (int)pos;
}

u32 bb_crc24(const u8 *data, u32 len, u32 init, const bt_addr *master)
{
    u32 crc = master ? (u32)master->b[0] : init;

    (void)init;

    for (u32 i = 0; i < len; i++) {
        for (int bit = 0; bit < 8; bit++) {
            u32 in = (data[i] >> bit) & 1;
            u32 feedback = ((crc >> 23) & 1) ^ in;
            crc = (crc << 1) & 0xFFFFFF;
            if (feedback)
                crc ^= 0x00065B;
        }
    }
    return crc;
}

u16 bb_crc16(const u8 *data, u32 len, u16 init)
{
    u16 crc = init;

    for (u32 i = 0; i < len; i++) {
        for (int bit = 0; bit < 8; bit++) {
            u16 in = (data[i] >> bit) & 1;
            u16 feedback = ((crc >> 15) & 1) ^ in;
            crc = (u16)(crc << 1);
            if (feedback)
                crc ^= 0x1021;
        }
    }
    return crc;
}

static u8 whiten_lfsr(u8 *state)
{
    u8 feedback = (u8)(((*state >> 6) ^ (*state >> 3)) & 1);
    *state = (u8)((*state << 1) | feedback);
    return *state;
}

void bb_whiten(u8 *data, u32 len, u8 clock6, u8 channel)
{
    u8 lfsr = (u8)(0x40 | ((clock6 & 0x3F) ^ (channel & 0x3F)));

    for (u32 i = 0; i < len; i++) {
        for (int bit = 0; bit < 8; bit++) {
            if (whiten_lfsr(&lfsr) & 1)
                data[i] ^= (u8)(1u << bit);
        }
    }
}

void bb_dewhiten(u8 *data, u32 len, u8 clock6, u8 channel)
{
    bb_whiten(data, len, clock6, channel);
}

u32 bb_fec13_encode(const u8 *in, u32 bytes, u8 *out)
{
    u32 bit = 0;

    for (u32 i = 0; i < bytes; i++) {
        for (int b = 0; b < 8; b++) {
            u8 v = (u8)((in[i] >> b) & 1);
            for (int r = 0; r < 3; r++) {
                u32 p = bit++;
                if (v)
                    out[p >> 3] |= (u8)(1u << (p & 7));
                else
                    out[p >> 3] &= (u8)~(1u << (p & 7));
            }
        }
    }
    return (bit + 7) / 8;
}

u32 bb_fec13_decode(const u8 *in, u32 bytes, u8 *out)
{
    u32 bit = 0;
    u32 out_bits = bytes * 8 / 3;

    for (u32 i = 0; i < (out_bits + 7) / 8; i++)
        out[i] = 0;

    for (u32 b = 0; b < out_bits; b++) {
        u32 ones = 0;
        for (int r = 0; r < 3; r++) {
            u32 p = bit++;
            ones += (in[p >> 3] >> (p & 7)) & 1;
        }
        if (ones >= 2)
            out[b >> 3] |= (u8)(1u << (b & 7));
    }
    return (out_bits + 7) / 8;
}

enum bb_packet_type bb_packet_from_hci(u16 handle_type)
{
    switch (handle_type) {
    case 0x0:
        return BB_PKT_NULL;
    case 0x1:
        return BB_PKT_DM1;
    case 0x2:
        return BB_PKT_DH1;
    case 0x3:
        return BB_PKT_DM3;
    case 0x4:
        return BB_PKT_DH3;
    case 0x5:
        return BB_PKT_DM5;
    case 0x6:
        return BB_PKT_DH5;
    default:
        return BB_PKT_DM1;
    }
}
