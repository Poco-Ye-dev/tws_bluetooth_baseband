#ifndef __BT_LE_HW_H__
#define __BT_LE_HW_H__

#include <core/types.h>
#include <driver/regmap.h>

/*
 * LE hardware driver (the RF_ble5 equivalent).
 *
 * This layer programs the chip's BLE5 engine: connection anchors, window
 * widening, PHY selection (1M / 2M / Coded), channel selection, extended
 * advertising hardware and the TX/RX DMA rings.  Protocol state stays in
 * le_ll.c; this file only touches registers.
 */

#define LE_REG_CTRL        0x000
#define LE_REG_STATUS      0x004
#define LE_REG_INTERVAL    0x008
#define LE_REG_WINSIZE     0x00C
#define LE_REG_WIDEN       0x010
#define LE_REG_ANCHOR0     0x014
#define LE_REG_ANCHOR1     0x018
#define LE_REG_CHMAP0      0x01C
#define LE_REG_PHY         0x024
#define LE_REG_CHSEL       0x028
#define LE_REG_ADV_CTRL    0x02C
#define LE_REG_TXPTR       0x030
#define LE_REG_RXPTR       0x034
#define LE_REG_IRQ_EN      0x038
#define LE_REG_POWER       0x03C
#define LE_REG_IRQ_STATUS  0x040
#define LE_REG_IRQ_ACK     0x044
#define LE_REG_RXBUF       0x048
#define LE_REG_RXBUF_LEN   0x04C

#define LE_IRQ_ANCHOR      (1u << 0)
#define LE_IRQ_RX_DONE     (1u << 1)
#define LE_IRQ_TX_DONE     (1u << 2)
#define LE_IRQ_DMA         (1u << 3)
#define LE_IRQ_CTE         (1u << 4)

#define LE_CTRL_ENABLE     (1u << 0)
#define LE_CTRL_ANCHOR     (1u << 1)
#define LE_CTRL_ADV        (1u << 2)

#define LE_PHY_1M          0x01
#define LE_PHY_2M          0x02
#define LE_PHY_CODED       0x04

#define LE_HW_MAX          8

#define LE_PREAMBLE_1M_US  8
#define LE_AA_1M_US        32
#define LE_CRC_1M_US       24

struct le_hw_config {
    u16 interval;
    u16 win_size;
    u16 win_offset;
    u8 widen;
    u8 channel_map[5];
    u8 tx_phy;
    u8 rx_phy;
    u8 coded_s;
    u8 chsel;
    bool adv_primary;
    bool adv_secondary;
};

struct le_hw {
    const char *name;
    struct regmap regs;
    bool initialized;
    bool enabled;
    u32 anchor[2];
    struct le_hw_config config;
};

int le_hw_init(struct le_hw *hw, const char *name, void *base, u8 reg_bits);
struct le_hw *le_hw_alloc(const char *name, void *base);
void le_hw_free(struct le_hw *hw);
struct le_hw *le_hw_by_index(u8 index);
u8 le_hw_index(const struct le_hw *hw);
int le_hw_configure(struct le_hw *hw, const struct le_hw_config *config);
int le_hw_anchor_set(struct le_hw *hw, u8 index, u32 slot);
int le_hw_anchor_disable(struct le_hw *hw);
int le_hw_enable(struct le_hw *hw, bool enable);
int le_hw_set_phy(struct le_hw *hw, u8 tx_phy, u8 rx_phy, u8 coded_s);
int le_hw_set_chsel(struct le_hw *hw, u8 algorithm);
int le_hw_ext_adv(struct le_hw *hw, bool primary, bool enable);
int le_hw_tx_buffer(struct le_hw *hw, const void *data, u16 len);
int le_hw_rx_buffer(struct le_hw *hw, void *data, u16 cap);
int le_hw_power(struct le_hw *hw, bool on);
int le_hw_irq_enable(struct le_hw *hw, u32 mask);
u16 le_hw_conn_dead_slots(u16 max_pdu, u8 phy, u8 coded_s);
u32 le_hw_packet_us(u16 pdu_len, u8 phy, u8 coded_s);
u32 le_hw_irq_status(struct le_hw *hw);
int le_hw_irq_ack(struct le_hw *hw, u32 mask);
int le_hw_rx_read(struct le_hw *hw, u8 *data, u16 cap);

/* Per-chip entry points supplied by cpu/<chip>/ble5.c. */
int ble5_port_init(void);
struct le_hw *ble5_hw(void);

#endif /* __BT_LE_HW_H__ */
