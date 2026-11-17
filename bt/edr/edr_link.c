#include <bt/edr_link.h>
#include <bt/edr_irq.h>
#include <bt/baseband.h>
#include <bt/rf_coexist.h>
#include <core/string.h>

/*
 * EDR link layer wiring for the eight frame engines.
 *
 * The per-slot work is: timer fires -> open window -> pop TX ring into the
 * baseband ARQ -> baseband tick -> hardware IRQ feeds the RX ring.
 */

static struct edr_link links[EDR_LINK_MAX];
static u32 edr_slot;

static struct edr_link *by_handle(u16 handle)
{
    for (unsigned int i = 0; i < EDR_LINK_MAX; i++) {
        if (links[i].used && links[i].handle == handle)
            return &links[i];
    }
    return NULL;
}

struct edr_link *edr_link_by_hw(u8 hw_index)
{
    for (unsigned int i = 0; i < EDR_LINK_MAX; i++) {
        if (links[i].used && links[i].hw_index == hw_index)
            return &links[i];
    }
    return NULL;
}

static void window_open(void *arg)
{
    struct edr_link *link = arg;
    if (!link || !link->used)
        return;
    edr_hw_set_slot_trigger(link->hw, 1);
    edr_hw_power(link->hw, true);
    link->window_open = true;
}

static void irq_slot(u16 hw_index, u32 slot)
{
    struct edr_link *link = edr_link_by_hw((u8)hw_index);
    if (link)
        link->anchor = slot;
}

static void irq_rx(u16 hw_index, const u8 *data, u16 len)
{
    struct edr_link *link = edr_link_by_hw((u8)hw_index);
    if (!link)
        return;
    le_ring_push(&link->rx, data, len);
    link->window_open = false;
}

static void irq_tx(u16 hw_index)
{
    (void)hw_index;
}

static const struct edr_irq_ops edr_link_irq_ops = {
    .slot = irq_slot,
    .rx_frame = irq_rx,
    .tx_done = irq_tx,
};

int edr_link_init(void)
{
    memset(links, 0, sizeof(links));
    le_timer_init();

    for (u8 i = 0; i < EDR_HW_MAX; i++) {
        void *base = (void *)(uptr)(0x40090000u + (u32)i * 0x1000);
        char name[16] = { 'b', 't', '0' + i, '\0' };
        struct edr_hw *hw = edr_hw_alloc(name, base);
        if (!hw)
            return -1;
        edr_hw_rf_init(hw, 4);
        edr_hw_txbuf_config(hw, 0x40090000u + (u32)i * 0x1000 + 0x800, 1024);
        edr_hw_rxbuf_config(hw, 0x40090000u + (u32)i * 0x1000 + 0xC00, 1024);
        edr_hw_set_window(hw, 0x20, 0);
        edr_hw_packet_init(hw);
    }

    edr_irq_register_ops(&edr_link_irq_ops);
    return edr_irq_init();
}

struct edr_link *edr_link_open(u16 handle, const bt_addr *peer, u16 interval)
{
    (void)peer;

    for (unsigned int i = 0; i < EDR_LINK_MAX; i++) {
        struct edr_link *link = &links[i];
        if (link->used)
            continue;

        memset(link, 0, sizeof(*link));
        link->used = true;
        link->handle = handle;
        link->hw_index = (u8)i;
        link->hw = edr_hw_by_index((u8)i);
        link->interval_slots = interval ? interval : 6;
        le_ring_init(&link->tx, link->tx_storage, sizeof(link->tx_storage));
        le_ring_init(&link->rx, link->rx_storage, sizeof(link->rx_storage));
        link->window_timer = le_timer_alloc(window_open, link);
        link->anchor = link->interval_slots;
        if (link->window_timer)
            le_timer_arm(link->window_timer, link->anchor);
        return link;
    }
    return NULL;
}

void edr_link_close(u16 handle)
{
    struct edr_link *link = by_handle(handle);
    if (!link)
        return;
    if (link->window_timer)
        le_timer_free(link->window_timer);
    edr_hw_power(link->hw, false);
    edr_hw_free(link->hw);
    memset(link, 0, sizeof(*link));
}

int edr_link_send(u16 handle, const u8 *data, u16 len)
{
    struct edr_link *link = by_handle(handle);
    if (!link)
        return -1;
    return le_ring_push(&link->tx, data, len) == len ? 0 : -2;
}

u16 edr_link_read(u16 handle, void *data, u16 cap)
{
    struct edr_link *link = by_handle(handle);
    return link ? le_ring_pop(&link->rx, data, cap) : 0;
}

void edr_link_tick(u32 slot)
{
    edr_slot = slot;
    le_timer_tick(slot);

    for (unsigned int i = 0; i < EDR_LINK_MAX; i++) {
        struct edr_link *link = &links[i];
        u8 pdu[EDR_LINK_RING_SIZE];
        u16 len;

        if (!link->used)
            continue;

        if (link->window_open) {
            len = le_ring_pop(&link->tx, pdu, sizeof(pdu));
            if (len)
                bb_send_acl(link->handle, pdu, len);
            link->window_open = false;
        }

        if (link->window_timer)
            le_timer_arm(link->window_timer, link->anchor);
        link->anchor += link->interval_slots;
    }

    if (rf_acquire(RF_OWNER_EDR)) {
        bb_tick(slot);
        rf_release(RF_OWNER_EDR);
    }
}
