#include <bt/radio.h>
#include <core/errno.h>
#include <core/string.h>
#include <core/printk.h>

static const struct bb_radio *front_end;
static struct regmap *radio_regs;

int radio_register(const struct bb_radio *radio)
{
    if (!radio || !radio->tx)
        return -EINVAL;
    front_end = radio;
    return 0;
}

const struct bb_radio *radio_get(void)
{
    return front_end;
}

void radio_set_regmap(struct regmap *map)
{
    radio_regs = map;
}

struct regmap *radio_regmap(void)
{
    return radio_regs;
}

int radio_open(void)
{
    if (!front_end)
        return -ENODEV;
    return front_end->open ? front_end->open() : 0;
}

int radio_close(void)
{
    if (!front_end)
        return -ENODEV;
    return front_end->close ? front_end->close() : 0;
}

int radio_tune(u16 channel)
{
    if (!front_end || !front_end->set_channel)
        return -ENODEV;
    return front_end->set_channel(channel);
}

int radio_power(s8 dbm)
{
    if (!front_end || !front_end->set_power)
        return -ENODEV;
    return front_end->set_power(dbm);
}

int radio_tx(const u8 *data, u16 len)
{
    if (!front_end || !front_end->tx)
        return -ENODEV;
    return front_end->tx(data, len);
}

int radio_rx(u8 *data, u16 cap)
{
    if (!front_end || !front_end->rx)
        return -ENODEV;
    return front_end->rx(data, cap);
}

int radio_rssi(s8 *dbm)
{
    if (!front_end || !front_end->get_rssi)
        return -ENODEV;
    return front_end->get_rssi(dbm);
}
