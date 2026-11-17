#include <bt/edr_hw.h>
#include <core/string.h>
#include <core/errno.h>

static struct edr_hw hw_pool[EDR_HW_MAX];
static bool hw_used[EDR_HW_MAX];

struct edr_hw *edr_hw_alloc(const char *name, void *base)
{
    for (unsigned int i = 0; i < EDR_HW_MAX; i++) {
        if (hw_used[i])
            continue;
        if (edr_hw_init(&hw_pool[i], name, base, 32))
            return NULL;
        hw_used[i] = true;
        return &hw_pool[i];
    }
    return NULL;
}

void edr_hw_free(struct edr_hw *hw)
{
    for (unsigned int i = 0; i < EDR_HW_MAX; i++) {
        if (&hw_pool[i] == hw)
            hw_used[i] = false;
    }
}

struct edr_hw *edr_hw_by_index(u8 index)
{
    return index < EDR_HW_MAX ? &hw_pool[index] : NULL;
}

u8 edr_hw_index(const struct edr_hw *hw)
{
    for (unsigned int i = 0; i < EDR_HW_MAX; i++) {
        if (&hw_pool[i] == hw)
            return (u8)i;
    }
    return 0xFF;
}

int edr_hw_init(struct edr_hw *hw, const char *name, void *base, u8 reg_bits)
{
    if (!hw || !base)
        return -EINVAL;

    memset(hw, 0, sizeof(*hw));
    hw->name = name;
    if (regmap_init_mmio(&hw->regs, name, base, reg_bits, 32))
        return -EINVAL;

    hw->config.win_size = 0x20;
    hw->config.slot_trigger = 1;
    hw->initialized = true;
    return 0;
}

int edr_hw_rf_init(struct edr_hw *hw, s8 power)
{
    if (!hw)
        return -EINVAL;
    hw->config.tx_power = power;
    regmap_write(&hw->regs, EDR_REG_RF_CTRL, 0x01);
    regmap_write(&hw->regs, EDR_REG_POWER, (u8)power);
    return 0;
}

int edr_hw_set_channel(struct edr_hw *hw, u8 channel)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_RF_CHANNEL, channel);
}

int edr_hw_txbuf_config(struct edr_hw *hw, u32 addr, u16 len)
{
    if (!hw)
        return -EINVAL;
    hw->txbuf_addr = addr;
    regmap_write(&hw->regs, EDR_REG_TXBUF_CFG, addr);
    regmap_write(&hw->regs, EDR_REG_TXBUF_LEN, len);
    return 0;
}

int edr_hw_txbuf_update(struct edr_hw *hw, u16 len)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_TXBUF_LEN, len);
}

int edr_hw_rxbuf_config(struct edr_hw *hw, u32 addr, u16 len)
{
    if (!hw)
        return -EINVAL;
    hw->rxbuf_addr = addr;
    regmap_write(&hw->regs, EDR_REG_RXBUF_CFG, addr);
    regmap_write(&hw->regs, EDR_REG_RXBUF_LEN, len);
    return 0;
}

int edr_hw_rx_read(struct edr_hw *hw, u8 *data, u16 cap)
{
    u32 len;

    if (!hw || !data)
        return -EINVAL;

    len = regmap_read(&hw->regs, EDR_REG_RXBUF_LEN);
    if (len > cap)
        len = cap;
    regmap_read_bulk(&hw->regs, EDR_REG_RXBUF_DATA, data, len);
    return (int)len;
}

int edr_hw_set_slot_trigger(struct edr_hw *hw, u16 slot)
{
    if (!hw)
        return -EINVAL;
    hw->config.slot_trigger = slot;
    return regmap_write(&hw->regs, EDR_REG_SLOT_TRIG, slot);
}

int edr_hw_set_window(struct edr_hw *hw, u16 size, u16 offset)
{
    if (!hw)
        return -EINVAL;
    hw->config.win_size = size;
    hw->config.win_offset = offset;
    regmap_write(&hw->regs, EDR_REG_WIN_SIZE, size);
    return regmap_write(&hw->regs, EDR_REG_WIN_OFFSET, offset);
}

u32 edr_hw_clkn(struct edr_hw *hw)
{
    return hw ? regmap_read(&hw->regs, EDR_REG_CLKN) : 0;
}

int edr_hw_set_clkn_phase(struct edr_hw *hw, u16 bitphase)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_CLK_OFFSET, bitphase);
}

int edr_hw_rxoffset_cal(struct edr_hw *hw, u32 target, u32 rx, s16 bit)
{
    s32 delta;

    if (!hw)
        return -EINVAL;

    delta = (s32)(rx - target) * 1000 + bit;
    hw->config.clk_offset = (u32)delta;
    return regmap_write(&hw->regs, EDR_REG_CLK_OFFSET, (u32)delta);
}

int edr_hw_set_afh(struct edr_hw *hw, u8 index, const u8 *map)
{
    u32 value;

    if (!hw || !map || index > 4)
        return -EINVAL;

    value = (u32)map[0] | ((u32)map[1] << 8) |
            ((u32)map[2] << 16) | ((u32)map[3] << 24);
    regmap_write(&hw->regs, EDR_REG_AFH0 + index * 4, value);
    if (index == 4)
        regmap_write(&hw->regs, EDR_REG_AFH0 + 16, map[4]);
    return 0;
}

int edr_hw_get_afh(struct edr_hw *hw, u8 index, u8 *map)
{
    u32 value;

    if (!hw || !map || index > 4)
        return -EINVAL;

    value = regmap_read(&hw->regs, EDR_REG_AFH0 + index * 4);
    map[0] = (u8)value;
    map[1] = (u8)(value >> 8);
    map[2] = (u8)(value >> 16);
    map[3] = (u8)(value >> 24);
    if (index == 4)
        map[4] = (u8)regmap_read(&hw->regs, EDR_REG_AFH0 + 16);
    return 0;
}

int edr_hw_packet_init(struct edr_hw *hw)
{
    if (!hw)
        return -EINVAL;
    /* Packet length/type defaults: DM1, 1 slot, CRC on. */
    return regmap_write(&hw->regs, EDR_REG_PACKET_CFG, 0x0001);
}

int edr_hw_fhs_write(struct edr_hw *hw, u32 addr)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_FHS_ADDR, addr);
}

int edr_hw_fhs_read(struct edr_hw *hw, u32 addr)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_FHS_ADDR, addr);
}

int edr_hw_connect(struct edr_hw *hw, u8 role)
{
    if (!hw)
        return -EINVAL;
    hw->config.role = role;
    return regmap_write(&hw->regs, EDR_REG_ROLE, role);
}

int edr_hw_power(struct edr_hw *hw, bool on)
{
    if (!hw)
        return -EINVAL;
    hw->enabled = on;
    return regmap_write(&hw->regs, EDR_REG_CTRL, on ? 1 : 0);
}

int edr_hw_irq_enable(struct edr_hw *hw, u32 mask)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_IRQ_EN, mask);
}

u32 edr_hw_irq_status(struct edr_hw *hw)
{
    return hw ? regmap_read(&hw->regs, EDR_REG_IRQ_STATUS) : 0;
}

int edr_hw_irq_ack(struct edr_hw *hw, u32 mask)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, EDR_REG_IRQ_ACK, mask);
}
