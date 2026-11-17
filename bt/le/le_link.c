#include <bt/le_link.h>
#include <bt/le_irq.h>
#include <bt/rf_coexist.h>
#include "../common/bb_internal.h"
#include <bt/radio.h>
#include <core/string.h>

/*
 * LE link wiring: 8 HW instances, per-link anchor timer and two SPSC rings.
 * This is the architecture the controller uses: the timer opens the window,
 * the RX interrupt fills the ring, and the host drains the ring.
 */

static struct le_link links[LE_LINK_MAX];
static u32 link_slot;

static struct le_link *link_for_handle(u16 handle)
{
    for (unsigned int i = 0; i < LE_LINK_MAX; i++) {
        if (links[i].used && links[i].handle == handle)
            return &links[i];
    }
    return NULL;
}

struct le_link *le_link_by_hw(u8 hw_index)
{
    for (unsigned int i = 0; i < LE_LINK_MAX; i++) {
        if (links[i].used && links[i].hw_index == hw_index)
            return &links[i];
    }
    return NULL;
}

static void window_open(void *arg)
{
    struct le_link *link = arg;

    if (!link || !link->used)
        return;

    le_hw_anchor_set(link->hw, 0, link->anchor);
    le_hw_enable(link->hw, true);
    link->window_open = true;
}

static void irq_anchor(u16 handle)
{
    struct le_link *link = le_link_by_hw((u8)handle);
    if (link)
        window_open(link);
}

static void irq_rx(u16 handle, const u8 *pdu, u16 len)
{
    struct le_link *link = le_link_by_hw((u8)handle);

    if (!link)
        return;

    le_ring_push(&link->rx, pdu, len);
    if (link->conn)
        le_ll_rx(link->conn, pdu, len);
    link->window_open = false;
}

static void irq_tx(u16 handle)
{
    struct le_link *link = le_link_by_hw((u8)handle);
    if (link)
        le_ll_tick(link_slot);
}

static struct le_irq_ops link_irq_ops = {
    .anchor = irq_anchor,
    .rx_done = irq_rx,
    .tx_done = irq_tx,
};

int le_link_init(void)
{
    memset(links, 0, sizeof(links));
    le_timer_init();

    for (u8 i = 0; i < LE_HW_MAX; i++) {
        /* Simulated per-instance register window: base + index * 0x1000. */
        void *base = (void *)(uptr)(0x40080000u + (u32)i * 0x1000);
        char name[16] = { 'l', 'e', '0' + i, '\0' };
        struct le_hw *hw = le_hw_alloc(name, base);
        if (!hw)
            return -1;
    }

    le_irq_register_ops(&link_irq_ops);
    return le_irq_init();
}

struct le_link *le_link_open(u16 handle, const bt_addr *peer,
                             u16 interval, u16 latency, u16 timeout)
{
    for (unsigned int i = 0; i < LE_LINK_MAX; i++) {
        struct le_link *link = &links[i];
        if (link->used)
            continue;

        memset(link, 0, sizeof(*link));
        link->used = true;
        link->handle = handle;
        link->hw_index = (u8)i;
        link->hw = le_hw_by_index((u8)i);
        link->interval_slots = (u16)((u32)interval * 16 / 10);
        link->winsize = 2;
        le_ring_init(&link->tx, link->tx_storage, sizeof(link->tx_storage));
        le_ring_init(&link->rx, link->rx_storage, sizeof(link->rx_storage));

        link->conn = le_ll_create(handle, peer, 0x8E89BED6u,
                                  0x555555u, interval, latency, timeout);
        link->window_timer = le_timer_alloc(window_open, link);
        link->anchor = interval ? link->interval_slots : 6;
        if (link->window_timer)
            le_timer_arm(link->window_timer, link->anchor);
        return link;
    }
    return NULL;
}

void le_link_close(u16 handle)
{
    struct le_link *link = link_for_handle(handle);
    if (!link)
        return;
    if (link->window_timer)
        le_timer_free(link->window_timer);
    le_hw_enable(link->hw, false);
    le_hw_free(link->hw);
    memset(link, 0, sizeof(*link));
}

int le_link_send(u16 handle, const u8 *data, u16 len)
{
    struct le_link *link = link_for_handle(handle);

    if (!link)
        return -1;
    if (le_ring_push(&link->tx, data, len) != len)
        return -2;
    return 0;
}

u16 le_link_read(u16 handle, void *data, u16 cap)
{
    struct le_link *link = link_for_handle(handle);
    return link ? le_ring_pop(&link->rx, data, cap) : 0;
}

void le_link_tick(u32 slot)
{
    link_slot = slot;
    le_timer_tick(slot);

    for (unsigned int i = 0; i < LE_LINK_MAX; i++) {
        struct le_link *link = &links[i];
        u8 pdu[252];
        u16 len;

        if (!link->used)
            continue;

        if (link->window_open) {
            len = le_ring_pop(&link->tx, pdu, sizeof(pdu));
            if (len && link->conn)
                le_ll_send(link->conn, pdu, len);
            le_hw_enable(link->hw, false);
            link->window_open = false;
        }

        le_hw_anchor_set(link->hw, 0, link->anchor);
        if (link->window_timer)
            le_timer_arm(link->window_timer, link->anchor);
        link->anchor += link->interval_slots ? link->interval_slots : 6;
    }

    le_ll_tick(slot);
    le_hw_enable(le_hw_by_index(0), true);
}

int le_stack_init(void)
{
    bb_le_init(NULL, NULL);
    return le_link_init();
}

void le_stack_tick(u32 slot)
{
    if (!rf_acquire(RF_OWNER_LE))
        return;
    bb_le_tick(slot);
    le_link_tick(slot);
    rf_release(RF_OWNER_LE);
}
