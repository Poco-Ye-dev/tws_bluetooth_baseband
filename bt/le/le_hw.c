#include <bt/le_hw.h>
#include <bt/radio.h>
#include <core/string.h>
#include <core/errno.h>

/*
 * Register level LE engine driver.
 *
 * 1M uncoded timing uses the standard preamble/AA/PDU/CRC layout; 2M halves
 * everything; Coded PHY multiplies the PDU and CRC by the coding rate S
 * (S=8) and adds the coding indicator and termination fields.
 */

static struct le_hw hw_pool[LE_HW_MAX];
static bool hw_used[LE_HW_MAX];

struct le_hw *le_hw_alloc(const char *name, void *base)
{
    for (unsigned int i = 0; i < LE_HW_MAX; i++) {
        if (hw_used[i])
            continue;
        if (le_hw_init(&hw_pool[i], name, base, 32))
            return NULL;
        hw_used[i] = true;
        return &hw_pool[i];
    }
    return NULL;
}

void le_hw_free(struct le_hw *hw)
{
    for (unsigned int i = 0; i < LE_HW_MAX; i++) {
        if (&hw_pool[i] == hw)
            hw_used[i] = false;
    }
}

struct le_hw *le_hw_by_index(u8 index)
{
    return index < LE_HW_MAX ? &hw_pool[index] : NULL;
}

u8 le_hw_index(const struct le_hw *hw)
{
    for (unsigned int i = 0; i < LE_HW_MAX; i++) {
        if (&hw_pool[i] == hw)
            return (u8)i;
    }
    return 0xFF;
}

int le_hw_init(struct le_hw *hw, const char *name, void *base, u8 reg_bits)
{
    if (!hw || !base)
        return -EINVAL;

    memset(hw, 0, sizeof(*hw));
    hw->name = name;
    if (regmap_init_mmio(&hw->regs, name, base, reg_bits, 32))
        return -EINVAL;

    hw->config.interval = 6;
    hw->config.tx_phy = LE_PHY_1M;
    hw->config.rx_phy = LE_PHY_1M;
    hw->config.chsel = 2;
    memset(hw->config.channel_map, 0xFF, sizeof(hw->config.channel_map));
    hw->initialized = true;
    return 0;
}

u32 le_hw_packet_us(u16 pdu_len, u8 phy, u8 coded_s)
{
    u32 preamble, aa, pdu, crc;

    if (phy & LE_PHY_CODED) {
        u32 s = coded_s ? coded_s : 8;
        preamble = 80;
        aa = 256;
        pdu = 16 + 24 + (u32)pdu_len * 8 * s;
        crc = 24 * s + 3 * s;
        return preamble + aa + pdu + crc;
    }

    preamble = (phy & LE_PHY_2M) ? LE_PREAMBLE_1M_US / 2 : LE_PREAMBLE_1M_US;
    aa = (phy & LE_PHY_2M) ? LE_AA_1M_US / 2 : LE_AA_1M_US;
    pdu = (phy & LE_PHY_2M) ? (u32)pdu_len * 4 : (u32)pdu_len * 8;
    crc = (phy & LE_PHY_2M) ? LE_CRC_1M_US / 2 : LE_CRC_1M_US;
    return preamble + aa + pdu + crc;
}

u16 le_hw_conn_dead_slots(u16 max_pdu, u8 phy, u8 coded_s)
{
    u32 us = le_hw_packet_us(max_pdu, phy, coded_s);
    return (u16)((us + 624) / 625 + 2);
}

int le_hw_configure(struct le_hw *hw, const struct le_hw_config *config)
{
    if (!hw || !config)
        return -EINVAL;

    hw->config = *config;

    regmap_write(&hw->regs, LE_REG_INTERVAL, config->interval);
    regmap_write(&hw->regs, LE_REG_WINSIZE,
                 ((u32)config->win_size << 16) | config->win_offset);
    regmap_write(&hw->regs, LE_REG_WIDEN, config->widen);
    regmap_write(&hw->regs, LE_REG_CHMAP0,
                 (u32)config->channel_map[0] |
                 ((u32)config->channel_map[1] << 8) |
                 ((u32)config->channel_map[2] << 16) |
                 ((u32)config->channel_map[3] << 24));
    regmap_write(&hw->regs, LE_REG_CHMAP0 + 4, config->channel_map[4]);

    le_hw_set_phy(hw, config->tx_phy, config->rx_phy, config->coded_s);
    le_hw_set_chsel(hw, config->chsel);
    return 0;
}

int le_hw_anchor_set(struct le_hw *hw, u8 index, u32 slot)
{
    if (!hw || index > 1)
        return -EINVAL;
    hw->anchor[index] = slot;
    regmap_write(&hw->regs, index ? LE_REG_ANCHOR1 : LE_REG_ANCHOR0, slot);
    regmap_update_bits(&hw->regs, LE_REG_CTRL, LE_CTRL_ANCHOR, LE_CTRL_ANCHOR);
    return 0;
}

int le_hw_anchor_disable(struct le_hw *hw)
{
    if (!hw)
        return -EINVAL;
    regmap_update_bits(&hw->regs, LE_REG_CTRL, LE_CTRL_ANCHOR, 0);
    return 0;
}

int le_hw_enable(struct le_hw *hw, bool enable)
{
    if (!hw)
        return -EINVAL;
    hw->enabled = enable;
    regmap_update_bits(&hw->regs, LE_REG_CTRL, LE_CTRL_ENABLE,
                       enable ? LE_CTRL_ENABLE : 0);
    return 0;
}

int le_hw_set_phy(struct le_hw *hw, u8 tx_phy, u8 rx_phy, u8 coded_s)
{
    u32 value;

    if (!hw)
        return -EINVAL;

    value = (u32)(tx_phy & 0x07) | ((u32)(rx_phy & 0x07) << 4) |
            ((u32)(coded_s & 0x07) << 8);
    hw->config.tx_phy = tx_phy;
    hw->config.rx_phy = rx_phy;
    hw->config.coded_s = coded_s;
    return regmap_write(&hw->regs, LE_REG_PHY, value);
}

int le_hw_set_chsel(struct le_hw *hw, u8 algorithm)
{
    if (!hw)
        return -EINVAL;
    hw->config.chsel = algorithm ? algorithm : 2;
    return regmap_write(&hw->regs, LE_REG_CHSEL, hw->config.chsel);
}

int le_hw_ext_adv(struct le_hw *hw, bool primary, bool enable)
{
    u32 mask = primary ? LE_CTRL_ADV : (LE_CTRL_ADV << 4);
    u32 value = enable ? mask : 0;

    if (!hw)
        return -EINVAL;
    return regmap_update_bits(&hw->regs, LE_REG_ADV_CTRL, mask, value);
}

int le_hw_tx_buffer(struct le_hw *hw, const void *data, u16 len)
{
    if (!hw || !data)
        return -EINVAL;
    regmap_write(&hw->regs, LE_REG_TXPTR, (u32)(uptr)data);
    regmap_write(&hw->regs, LE_REG_TXPTR + 4, len);
    return 0;
}

int le_hw_rx_buffer(struct le_hw *hw, void *data, u16 cap)
{
    if (!hw || !data)
        return -EINVAL;
    regmap_write(&hw->regs, LE_REG_RXPTR, (u32)(uptr)data);
    regmap_write(&hw->regs, LE_REG_RXPTR + 4, cap);
    return 0;
}

int le_hw_power(struct le_hw *hw, bool on)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, LE_REG_POWER, on ? 1 : 0);
}

int le_hw_irq_enable(struct le_hw *hw, u32 mask)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, LE_REG_IRQ_EN, mask);
}

u32 le_hw_irq_status(struct le_hw *hw)
{
    return hw ? regmap_read(&hw->regs, LE_REG_IRQ_STATUS) : 0;
}

int le_hw_irq_ack(struct le_hw *hw, u32 mask)
{
    if (!hw)
        return -EINVAL;
    return regmap_write(&hw->regs, LE_REG_IRQ_ACK, mask);
}

int le_hw_rx_read(struct le_hw *hw, u8 *data, u16 cap)
{
    u32 len;

    if (!hw || !data)
        return -EINVAL;

    len = regmap_read(&hw->regs, LE_REG_RXBUF_LEN);
    if (len > cap)
        len = cap;
    regmap_read_bulk(&hw->regs, LE_REG_RXBUF, data, len);
    return (int)len;
}
