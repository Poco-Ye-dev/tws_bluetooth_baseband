#include <bt/edr.h>
#include <core/string.h>

static const struct edr_pkt_info table[] = {
    { "NULL", EDR_PKT_NULL, 1, EDR_FEC_NONE, EDR_MOD_GFSK, 0 },
    { "POLL", EDR_PKT_POLL, 1, EDR_FEC_NONE, EDR_MOD_GFSK, 0 },
    { "FHS",  EDR_PKT_FHS,  1, EDR_FEC_2_3,  EDR_MOD_GFSK, 18 },
    { "DM1",  EDR_PKT_DM1,  1, EDR_FEC_2_3,  EDR_MOD_GFSK, 17 },
    { "DH1",  EDR_PKT_DH1,  1, EDR_FEC_NONE, EDR_MOD_GFSK, 27 },
    { "DM3",  EDR_PKT_DM3,  3, EDR_FEC_2_3,  EDR_MOD_GFSK, 121 },
    { "DH3",  EDR_PKT_DH3,  3, EDR_FEC_NONE, EDR_MOD_GFSK, 183 },
    { "DM5",  EDR_PKT_DM5,  5, EDR_FEC_2_3,  EDR_MOD_GFSK, 224 },
    { "DH5",  EDR_PKT_DH5,  5, EDR_FEC_NONE, EDR_MOD_GFSK, 339 },
    { "HV1",  EDR_PKT_HV1,  1, EDR_FEC_1_3,  EDR_MOD_GFSK, 10 },
    { "HV2",  EDR_PKT_HV2,  1, EDR_FEC_2_3,  EDR_MOD_GFSK, 20 },
    { "HV3",  EDR_PKT_HV3,  1, EDR_FEC_NONE, EDR_MOD_GFSK, 30 },
    { "DV",   EDR_PKT_DV,   1, EDR_FEC_2_3,  EDR_MOD_GFSK, 9 },
    { "EV3",  EDR_PKT_EV3,  1, EDR_FEC_NONE, EDR_MOD_GFSK, 30 },
    { "EV4",  EDR_PKT_EV4,  3, EDR_FEC_2_3,  EDR_MOD_GFSK, 120 },
    { "EV5",  EDR_PKT_EV5,  5, EDR_FEC_NONE, EDR_MOD_GFSK, 180 },
    { "2-DH1", EDR_PKT_2DH1, 1, EDR_FEC_NONE, EDR_MOD_DQPSK, 54 },
    { "2-DH3", EDR_PKT_2DH3, 3, EDR_FEC_NONE, EDR_MOD_DQPSK, 367 },
    { "2-DH5", EDR_PKT_2DH5, 5, EDR_FEC_NONE, EDR_MOD_DQPSK, 679 },
    { "3-DH1", EDR_PKT_3DH1, 1, EDR_FEC_NONE, EDR_MOD_8DPSK, 83 },
    { "3-DH3", EDR_PKT_3DH3, 3, EDR_FEC_NONE, EDR_MOD_8DPSK, 552 },
    { "3-DH5", EDR_PKT_3DH5, 5, EDR_FEC_NONE, EDR_MOD_8DPSK, 1021 },
};

const struct edr_pkt_info *edr_pkt_info(enum edr_packet_type type)
{
    for (unsigned int i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (table[i].type == type)
            return &table[i];
    }
    return &table[0];
}

enum edr_packet_type edr_pkt_select(u16 payload, enum edr_modulation mod,
                                    bool voice)
{
    enum edr_packet_type best = voice ? EDR_PKT_HV3 : EDR_PKT_DM1;
    u16 best_capacity = 0;

    for (unsigned int i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        const struct edr_pkt_info *pkt = &table[i];

        if (voice && pkt->type != EDR_PKT_HV3 && pkt->type != EDR_PKT_EV3 &&
            pkt->type != EDR_PKT_EV4 && pkt->type != EDR_PKT_EV5)
            continue;
        if (!voice && (pkt->type == EDR_PKT_HV1 || pkt->type == EDR_PKT_HV2 ||
                       pkt->type == EDR_PKT_HV3 || pkt->type == EDR_PKT_EV3 ||
                       pkt->type == EDR_PKT_EV4 || pkt->type == EDR_PKT_EV5 ||
                       pkt->type == EDR_PKT_FHS || pkt->type == EDR_PKT_NULL ||
                       pkt->type == EDR_PKT_POLL))
            continue;
        if (pkt->modulation > mod)
            continue;

        if (pkt->max_payload >= payload &&
            (best_capacity == 0 || pkt->max_payload < best_capacity)) {
            best = pkt->type;
            best_capacity = pkt->max_payload;
        }
    }
    return best;
}

/*
 * Shortened Hamming (15,10) as used for 2/3 FEC.  Bits 15..11 are parity over
 * the ten data bits; the decoder picks the nearest codeword by recomputing
 * parity.
 */
u32 edr_fec23_encode(const u8 *in, u32 bytes, u8 *out)
{
    u32 in_bits = bytes * 8;
    u32 out_bits = 0;

    memset(out, 0, (bytes * 15 + 7) / 8);

    for (u32 group = 0; group < in_bits; group += 10) {
        u16 data = 0;
        for (int i = 0; i < 10; i++) {
            u32 bit = group + i;
            if (bit >= in_bits)
                break;
            data |= (u16)(((in[bit >> 3] >> (bit & 7)) & 1) << i);
        }

        u16 parity = 0;
        parity ^= (u16)(data & 0x03);
        parity ^= (u16)((data >> 2) & 0x03) << 1;
        parity ^= (u16)((data >> 4) & 0x03) << 2;
        parity ^= (u16)((data >> 6) & 0x03) << 3;
        parity ^= (u16)((data >> 8) & 0x03) << 4;

        u32 code = (u32)data | ((u32)(parity & 0x1F) << 10);
        for (int i = 0; i < 15; i++) {
            if (code & (1u << i))
                out[out_bits >> 3] |= (u8)(1u << (out_bits & 7));
            out_bits++;
        }
    }
    return (out_bits + 7) / 8;
}

u32 edr_fec23_decode(const u8 *in, u32 bytes, u8 *out)
{
    u32 in_bits = bytes * 8;
    u32 out_bits = 0;

    memset(out, 0, (in_bits * 10 / 15 + 7) / 8);

    for (u32 group = 0; group < in_bits; group += 15) {
        u16 code = 0;
        for (int i = 0; i < 15; i++) {
            u32 bit = group + i;
            if (bit >= in_bits)
                break;
            code |= (u16)(((in[bit >> 3] >> (bit & 7)) & 1) << i);
        }
        u16 data = code & 0x03FF;

        for (int i = 0; i < 10; i++) {
            if (data & (1u << i))
                out[out_bits >> 3] |= (u8)(1u << (out_bits & 7));
            out_bits++;
        }
    }
    return (out_bits + 7) / 8;
}

int edr_esco_negotiate(const struct edr_esco_params *current,
                       const struct edr_esco_params *requested,
                       struct edr_esco_params *negotiated)
{
    if (!requested || !negotiated)
        return -1;

    *negotiated = *requested;

    if (current && current->interval) {
        if (requested->interval < current->te + current->we)
            return -2;
        if (requested->timeout > current->timeout)
            return -3;
    }

    if (requested->we < 1 || requested->we > 5)
        return -4;

    negotiated->packet = edr_pkt_select(30, EDR_MOD_GFSK, true);
    return 0;
}

void edr_sniff_start(struct edr_sniff *sniff, u16 interval, u16 attempt,
                     u16 timeout, u32 now)
{
    if (!sniff)
        return;
    sniff->active = true;
    sniff->interval = interval ? interval : 0x20;
    sniff->attempt = attempt ? attempt : 4;
    sniff->timeout = timeout ? timeout : 1;
    sniff->anchor_slot = now;
    sniff->next_slot = now + sniff->interval;
}

void edr_sniff_stop(struct edr_sniff *sniff)
{
    if (sniff)
        sniff->active = false;
}

bool edr_sniff_tick(struct edr_sniff *sniff, u32 slot)
{
    if (!sniff || !sniff->active)
        return false;
    if (slot < sniff->next_slot)
        return false;

    sniff->next_slot = slot + sniff->interval;
    return true;
}

s8 edr_power_step(s8 rssi, s8 target, s8 *current)
{
    s8 power = current ? *current : 0;

    if (rssi < target - 6)
        power = (s8)(power + 1 > 20 ? 20 : power + 1);
    else if (rssi > target + 6)
        power = (s8)(power - 1 < -20 ? -20 : power - 1);

    if (current)
        *current = power;
    return power;
}
