#include "../common/bb_internal.h"
#include "arq.h"
#include <bt/packet.h>
#include <bt/edr.h>
#include <bt/lmp.h>
#include <bt/frame.h>
#include <bt/radio.h>
#include <bt/edr_hw.h>
#include <bt/edr_link.h>
#include <bt/rf_coexist.h>
#include <bt/edr_hw.h>
#include <bt/edr_link.h>
#include <core/string.h>
#include <core/errno.h>
#include <core/printk.h>

/*
 * Link controller state machine.
 *
 * bb_tick() is the only time source: it is called once per 625 us slot by
 * the platform timer.  Every state either schedules a transmit, a receive or
 * a scan window; no other code touches the radio.
 */

struct bb_controller {
    struct bb_config config;
    const struct bb_ops *ops;
    enum bb_state state;
    struct bb_link links[BB_MAX_LINKS];
    struct bb_stats stats;
    u32 slots;
    u32 inquiry_until;
    u16 next_handle;
    u8 tx_power;
    bool inquiry_scan;
    bool page_scan;
};

static struct bb_controller bb;

#define LMP_WIRE_TAG 0x7E

static int lmp_send_adapter(struct lmp_link *link, u8 opcode,
                            const void *data, u16 len)
{
    return bb_lmp_send(link->handle, opcode, data, len);
}

static void lmp_sniff_cb(struct lmp_link *link, bool on)
{
    if (on)
        bb_sniff_enable(link->handle, 0x20, 4, 1);
    else
        bb_sniff_disable(link->handle);
}

static void lmp_esco_cb(struct lmp_link *link, const struct edr_esco_params *params)
{
    bb_esco_add(link->handle, params ? params->interval : 6);
}

static const struct lmp_ops bb_lmp_ops = {
    .send = lmp_send_adapter,
    .sniff = lmp_sniff_cb,
    .esco = lmp_esco_cb,
};

int bb_lmp_send(u16 handle, u8 opcode, const void *data, u16 len)
{
    struct ctrl_frame *frame = edr_frame_alloc(handle, (u8)(handle & 7));
    if (!frame)
        return -1;
    frame->type = 0xFF;             /* LMP marker */
    frame->data[0] = LMP_WIRE_TAG;
    frame->data[1] = opcode;
    if (len && data)
        memcpy(&frame->data[2], data, len);
    frame->len = (u16)(2 + len);
    return edr_frame_enqueue(frame, FRAME_PRIO_HIGH);
}

struct bb_link *bb_link_alloc(const bt_addr *peer, enum bb_link_type type)
{
    for (unsigned int i = 0; i < BB_MAX_LINKS; i++) {
        struct bb_link *link = &bb.links[i];
        if (link->used)
            continue;

        memset(link, 0, sizeof(*link));
        link->used = true;
        link->handle = bb.next_handle++;
        if (bb.next_handle < 0x0008)
            bb.next_handle = 0x0008;
        link->peer = *peer;
        link->type = type;
        link->last_slot = bb.slots;
        return link;
    }
    return NULL;
}

void bb_link_free(struct bb_link *link)
{
    if (link)
        memset(link, 0, sizeof(*link));
}

const struct bb_ops *bb_ops(void)
{
    return bb.ops;
}

const bt_addr *bb_local_addr(void)
{
    return &bb.config.local;
}

s8 bb_tx_power(void)
{
    return bb.tx_power;
}

struct bb_stats *bb_stats_mut(void)
{
    return &bb.stats;
}

int bb_init(const struct bb_config *config, const struct bb_ops *ops)
{
    if (!config || !config->local.b[0])
        return -EINVAL;

    memset(&bb, 0, sizeof(bb));
    bb.config = *config;
    if (!bb.config.acl_mtu)
        bb.config.acl_mtu = BB_MAX_ACL_MTU;
    if (!bb.config.max_retries)
        bb.config.max_retries = BB_MAX_RETRIES;
    bb.ops = ops;
    bb.state = BB_STATE_STANDBY;
    bb.next_handle = 0x0008;
    edr_afh_init();
    edr_sched_init();
    rf_coexist_init();
    edr_link_init();
    bb.tx_power = config->tx_power;

    bb_arq_init();
    edr_frame_init();
    lmp_init(&bb_lmp_ops);
    bb_sco_init();
    bb_afh_set(config->channel_map[0] ? config->channel_map : NULL);
    return 0;
}

int bb_start(void)
{
    int ret = radio_open();
    if (ret)
        return ret;

    radio_power(bb.tx_power);
    bb.state = BB_STATE_STANDBY;
    return 0;
}

int bb_stop(void)
{
    bb.state = BB_STATE_STANDBY;
    return radio_close();
}

int bb_set_tx_power(s8 dbm)
{
    bb.tx_power = dbm;
    return radio_power(dbm);
}

int bb_sniff_enable(u16 handle, u16 interval, u16 attempt, u16 timeout)
{
    struct bb_link *link = bb_link_find(handle);
    if (!link)
        return -ENOENT;
    link->sniff_active = true;
    link->sniff_interval = interval ? interval : 0x20;
    link->sniff_attempt = attempt ? attempt : 4;
    link->sniff_next = bb.slots + link->sniff_interval;
    (void)timeout;
    return 0;
}

int bb_sniff_disable(u16 handle)
{
    struct bb_link *link = bb_link_find(handle);
    if (!link)
        return -ENOENT;
    link->sniff_active = false;
    return 0;
}

int bb_esco_add(u16 handle, u16 interval)
{
    struct bb_link *link = bb_link_find(handle);
    if (!link)
        return -ENOENT;
    link->esco_active = true;
    link->esco_interval = interval ? interval : 6;
    return 0;
}

int bb_esco_remove(u16 handle)
{
    struct bb_link *link = bb_link_find(handle);
    if (!link)
        return -ENOENT;
    link->esco_active = false;
    return 0;
}

struct bb_link *bb_link_find(u16 handle)
{
    for (unsigned int i = 0; i < BB_MAX_LINKS; i++) {
        if (bb.links[i].used && bb.links[i].handle == handle)
            return &bb.links[i];
    }
    return NULL;
}

struct bb_link *bb_link_find_addr(const bt_addr *addr)
{
    for (unsigned int i = 0; i < BB_MAX_LINKS; i++) {
        if (bb.links[i].used && bt_addr_eq(&bb.links[i].peer, addr))
            return &bb.links[i];
    }
    return NULL;
}

int bb_connect(const bt_addr *peer)
{
    struct bb_link *link;

    if (!peer)
        return -EINVAL;
    if (bb_link_find_addr(peer))
        return -EALREADY;

    link = bb_link_alloc(peer, BB_LINK_ACL);
    if (!link)
        return -ENOMEM;

    lmp_link_open(link->handle, peer, 0);
    bb.state = BB_STATE_CONNECTED;
    if (bb.ops && bb.ops->link_up)
        bb.ops->link_up(link);
    return 0;
}

int bb_disconnect(u16 handle, u8 reason)
{
    struct bb_link *link = bb_link_find(handle);

    if (!link)
        return -ENOENT;

    bb_arq_flush(handle);
    lmp_link_close(handle);
    if (bb.ops && bb.ops->link_down)
        bb.ops->link_down(link, reason);
    bb_link_free(link);

    bool any = false;
    for (unsigned int i = 0; i < BB_MAX_LINKS; i++) {
        if (bb.links[i].used)
            any = true;
    }
    if (!any)
        bb.state = BB_STATE_STANDBY;
    return 0;
}

int bb_send_acl(u16 handle, const u8 *data, u16 len)
{
    if (!bb_link_find(handle))
        return -ENOENT;
    return bb_arq_push(handle, data, len);
}

int bb_send_sco(u16 handle, const u8 *data, u16 len)
{
    struct bb_link *link = bb_link_find(handle);
    if (!link)
        return -ENOENT;
    return bb_sco_send(link, data, len);
}

int bb_set_scan(bool inquiry, bool page)
{
    bb.inquiry_scan = inquiry;
    bb.page_scan = page;

    if (inquiry)
        bb.state = BB_STATE_INQUIRY_SCAN;
    else if (page)
        bb.state = BB_STATE_PAGE_SCAN;
    else if (bb.state == BB_STATE_INQUIRY_SCAN ||
             bb.state == BB_STATE_PAGE_SCAN)
        bb.state = BB_STATE_STANDBY;
    return 0;
}

int bb_start_inquiry(u32 duration_slots)
{
    bb.state = BB_STATE_INQUIRY;
    bb.inquiry_until = bb.slots + (duration_slots ? duration_slots : 0x2000);
    return 0;
}

static u16 channel_of(struct bb_link *link);

static void transmit_acl(struct bb_link *link)
{
    struct ctrl_frame *lmp = edr_frame_peek(link->handle);
    if (lmp) {
        u8 frame[4 + EDR_FRAME_PAYLOAD + 4];
        u8 header[4];
        u32 bits = 0;
        struct bb_header lh = {
            .lt_addr = (u8)(link->handle & 7),
            .type = BB_PKT_DM1,
            .flow = 1, .arqn = 1, .seqn = 0,
        };
        bb_header_encode(&lh, header, &bits);
        frame[0] = header[0]; frame[1] = header[1]; frame[2] = header[2];
        memcpy(&frame[3], lmp->data, lmp->len);
        u32 crc = bb_crc24(lmp->data, lmp->len, 0, &link->peer);
        frame[3 + lmp->len] = (u8)(crc & 0xFF);
        frame[4 + lmp->len] = (u8)((crc >> 8) & 0xFF);
        frame[5 + lmp->len] = (u8)((crc >> 16) & 0xFF);
        if (radio_tx(frame, (u16)(6 + lmp->len)) == 0)
            bb.stats.tx_packets++;
        edr_frame_free(edr_frame_dequeue(link->handle));
        return;
    }

    struct bb_arq_pkt *pkt = bb_arq_head(link->handle);
    struct bb_header hdr;
    u8 frame[4 + BB_MAX_ACL_MTU + 4];
    u8 header[4];
    u32 bits = 0;
    u8 channel;
    u16 air_len;

    if (!pkt)
        return;

    channel = channel_of(link);

    hdr.lt_addr = (u8)(link->handle & 0x07);
    hdr.type = (u8)edr_pkt_select(pkt->len, EDR_MOD_GFSK, false);
    hdr.flow = 1;
    hdr.arqn = 1;
    hdr.seqn = pkt->seqn;
    bb_header_encode(&hdr, header, &bits);

    frame[0] = header[0];
    frame[1] = header[1];
    frame[2] = header[2];

    const u8 *payload = pkt->data;
    struct lmp_link *llink = lmp_link_find(link->handle);
    if (llink && llink->encrypted) {
        if (!pkt->enc_ready) {
            if (lmp_encrypt(llink, pkt->data, pkt->cipher, pkt->len) == 0)
                pkt->enc_ready = true;
        }
        if (pkt->enc_ready)
            payload = pkt->cipher;
    }

    memcpy(&frame[3], payload, pkt->len);
    air_len = (u16)(3 + pkt->len);

    bb_whiten(&frame[3], pkt->len, (u8)(bb.slots & 0x3F), channel);

    u32 crc = bb_crc24(pkt->data, pkt->len, 0, &link->peer);
    frame[air_len++] = (u8)(crc & 0xFF);
    frame[air_len++] = (u8)((crc >> 8) & 0xFF);
    frame[air_len++] = (u8)((crc >> 16) & 0xFF);

    if (radio_tx(frame, air_len) == 0)
        bb.stats.tx_packets++;

    s8 rssi;
    if (radio_rssi(&rssi) == 0) {
        static s8 power = 4;
        edr_power_step(rssi, -50, &power);
        radio_power(power);
    }
}

static u16 channel_of(struct bb_link *link)
{
    return bb_hop_br(&link->peer, bb.slots);
}

static void receive_acl(struct bb_link *link)
{
    u8 frame[4 + BB_MAX_ACL_MTU + 4];
    int len = radio_rx(frame, sizeof(frame));

    if (len <= 0)
        return;

    u16 channel = channel_of(link);
    struct bb_header hdr;
    bb_header_decode(frame, 18, &hdr);

    u16 payload = (u16)(len - 6);
    if (payload > BB_MAX_ACL_MTU)
        payload = BB_MAX_ACL_MTU;

    bb_dewhiten(&frame[3], payload, (u8)(bb.slots & 0x3F), (u8)channel);

    struct lmp_link *llink = lmp_link_find(link->handle);
    if (llink && llink->encrypted) {
        u8 plain[BB_MAX_ACL_MTU];
        if (lmp_decrypt(llink, &frame[3], plain, payload) == 0)
            memcpy(&frame[3], plain, payload);
    }

    u32 crc = bb_crc24(&frame[3], payload, 0, &link->peer);
    u32 received = (u32)frame[3 + payload] |
                   ((u32)frame[4 + payload] << 8) |
                   ((u32)frame[5 + payload] << 16);

    if (crc != received) {
        bb.stats.crc_errors++;
        edr_afh_report((u16)channel, false);
        return;
    }

    edr_afh_report((u16)channel, true);

    bb.stats.rx_packets++;
    bb_arq_ack(link->handle, hdr.arqn);

    if (payload >= 2 && frame[3] == LMP_WIRE_TAG) {
        struct lmp_link *lmp = lmp_link_find(link->handle);
        if (lmp)
            lmp_rx(lmp, frame[4], &frame[5], (u16)(payload - 2));
        return;
    }

    if (bb.ops && bb.ops->acl_rx)
        bb.ops->acl_rx(link, &frame[3], payload);
}

void bb_tick(u32 slot)
{
    if (!rf_acquire(RF_OWNER_EDR))
        return;

    bb.slots = slot;

    if ((slot & 0x3FF) == 0) {
        u8 map[10];
        if (edr_afh_build_map(map) > 0) {
            bb_afh_set(map);
            if (bt_baseband_hw())
                edr_hw_set_afh(bt_baseband_hw(), 0, map);
        }
    }

    switch (bb.state) {
    case BB_STATE_CONNECTED:
        for (unsigned int i = 0; i < BB_MAX_LINKS; i++) {
            struct bb_link *link = &bb.links[i];
            if (!link->used)
                continue;
            if (link->sniff_active && slot < link->sniff_next)
                continue;
            if (link->sniff_active && slot >= link->sniff_next)
                link->sniff_next = slot + link->sniff_interval;
            radio_tune(channel_of(link));
            if (slot & 1)
                receive_acl(link);
            else
                transmit_acl(link);
            link->last_slot = slot;
        }
        break;
    case BB_STATE_INQUIRY:
        radio_tune(bb_hop_br(&bb.config.local, slot));
        if (slot >= bb.inquiry_until)
            bb.state = BB_STATE_STANDBY;
        break;
    case BB_STATE_PAGE:
        radio_tune(bb_hop_br(&bb.config.local, slot));
        break;
    case BB_STATE_PAGE_SCAN:
    case BB_STATE_INQUIRY_SCAN:
    case BB_STATE_STANDBY:
    default:
        break;
    }

    bb_sco_tick(slot);
    rf_release(RF_OWNER_EDR);
}

enum bb_state bb_state(void)
{
    return bb.state;
}

u32 bb_slots(void)
{
    return bb.slots;
}

const struct bb_config *bb_config(void)
{
    return &bb.config;
}

const struct bb_stats *bb_get_stats(void)
{
    return &bb.stats;
}
