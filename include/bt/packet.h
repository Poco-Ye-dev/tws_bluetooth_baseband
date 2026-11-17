#ifndef __BT_PACKET_H__
#define __BT_PACKET_H__

#include <core/types.h>
#include <bt/baseband.h>

/*
 * Baseband packet primitives: header coding, HEC, CRC, whitening and 1/3
 * rate FEC.  These are pure functions so they can be unit tested on a host.
 */

struct bb_header {
    u8 lt_addr;
    u8 type;
    u8 flow;
    u8 arqn;
    u8 seqn;
};

/* Header is 18 bits on air: 3 addr, 4 type, 1 flow, 1 arqn, 1 seqn, 8 HEC. */
void bb_header_encode(const struct bb_header *hdr, u8 *out, u32 *bits);
int bb_header_decode(const u8 *in, u32 bits, struct bb_header *hdr);

u8 bb_hec(u8 header10, const bt_addr *master);
u32 bb_crc24(const u8 *data, u32 len, u32 init, const bt_addr *master);
u16 bb_crc16(const u8 *data, u32 len, u16 init);

void bb_whiten(u8 *data, u32 len, u8 clock6, u8 channel);
void bb_dewhiten(u8 *data, u32 len, u8 clock6, u8 channel);

u32 bb_fec13_encode(const u8 *in, u32 bytes, u8 *out);
u32 bb_fec13_decode(const u8 *in, u32 bytes, u8 *out);

enum bb_packet_type bb_packet_from_hci(u16 handle_type);

#endif /* __BT_PACKET_H__ */
