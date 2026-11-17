#ifndef __BT_EDR_HW_H__
#define __BT_EDR_HW_H__

#include <core/types.h>
#include <driver/regmap.h>

/*
 * BR/EDR baseband hardware driver.
 *
 * Simulated register map for the classic engine: RF control, ACL TX/RX
 * buffers, slot trigger/window, clock phase calibration, AFH table, FHS
 * packet memory and interrupt control.
 */

#define EDR_HW_MAX 8

#define EDR_REG_CTRL        0x000
#define EDR_REG_STATUS      0x004
#define EDR_REG_RF_CTRL     0x008
#define EDR_REG_RF_CHANNEL  0x00C
#define EDR_REG_POWER       0x010
#define EDR_REG_SLOT_TRIG   0x014
#define EDR_REG_WIN_SIZE    0x018
#define EDR_REG_WIN_OFFSET  0x01C
#define EDR_REG_CLKN        0x020
#define EDR_REG_CLK_OFFSET  0x024
#define EDR_REG_TXBUF_CFG   0x028
#define EDR_REG_TXBUF_LEN   0x02C
#define EDR_REG_RXBUF_CFG   0x030
#define EDR_REG_RXBUF_LEN   0x034
#define EDR_REG_RXBUF_DATA  0x100
#define EDR_REG_AFH0        0x040
#define EDR_REG_PACKET_CFG  0x060
#define EDR_REG_FHS_ADDR    0x064
#define EDR_REG_IRQ_EN      0x068
#define EDR_REG_IRQ_STATUS  0x06C
#define EDR_REG_IRQ_ACK     0x070
#define EDR_REG_ROLE        0x074

#define EDR_IRQ_RX_DONE     (1u << 0)
#define EDR_IRQ_TX_DONE     (1u << 1)
#define EDR_IRQ_SLOT        (1u << 2)
#define EDR_IRQ_ERROR       (1u << 3)
#define EDR_IRQ_CONNECT     (1u << 4)

struct edr_hw_config {
    u8 role;
    s8 tx_power;
    u16 win_size;
    u16 win_offset;
    u16 slot_trigger;
    u32 clk_offset;
};

struct edr_hw {
    const char *name;
    struct regmap regs;
    struct edr_hw_config config;
    bool initialized;
    bool enabled;
    u32 txbuf_addr;
    u32 rxbuf_addr;
};

int edr_hw_init(struct edr_hw *hw, const char *name, void *base, u8 reg_bits);
struct edr_hw *edr_hw_alloc(const char *name, void *base);
void edr_hw_free(struct edr_hw *hw);
struct edr_hw *edr_hw_by_index(u8 index);
u8 edr_hw_index(const struct edr_hw *hw);
int edr_hw_rf_init(struct edr_hw *hw, s8 power);
int edr_hw_set_channel(struct edr_hw *hw, u8 channel);
int edr_hw_txbuf_config(struct edr_hw *hw, u32 addr, u16 len);
int edr_hw_txbuf_update(struct edr_hw *hw, u16 len);
int edr_hw_rxbuf_config(struct edr_hw *hw, u32 addr, u16 len);
int edr_hw_rx_read(struct edr_hw *hw, u8 *data, u16 cap);
int edr_hw_set_slot_trigger(struct edr_hw *hw, u16 slot);
int edr_hw_set_window(struct edr_hw *hw, u16 size, u16 offset);
u32 edr_hw_clkn(struct edr_hw *hw);
int edr_hw_set_clkn_phase(struct edr_hw *hw, u16 bitphase);
int edr_hw_rxoffset_cal(struct edr_hw *hw, u32 target, u32 rx, s16 bit);
int edr_hw_set_afh(struct edr_hw *hw, u8 index, const u8 *map);
int edr_hw_get_afh(struct edr_hw *hw, u8 index, u8 *map);
int edr_hw_packet_init(struct edr_hw *hw);
int edr_hw_fhs_write(struct edr_hw *hw, u32 addr);
int edr_hw_fhs_read(struct edr_hw *hw, u32 addr);
int edr_hw_connect(struct edr_hw *hw, u8 role);
int edr_hw_power(struct edr_hw *hw, bool on);
int edr_hw_irq_enable(struct edr_hw *hw, u32 mask);
u32 edr_hw_irq_status(struct edr_hw *hw);
int edr_hw_irq_ack(struct edr_hw *hw, u32 mask);

/* Weak per-chip entry; a port supplies the real instance. */
int bt_baseband_port_init(void);
struct edr_hw *bt_baseband_hw(void);

#endif /* __BT_EDR_HW_H__ */
