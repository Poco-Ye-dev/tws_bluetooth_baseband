#include <bt/le_iso.h>
#include <bt/radio.h>
#include <core/string.h>

static struct le_iso channels[LE_ISO_MAX_CHANNELS];

void le_iso_init(void)
{
    memset(channels, 0, sizeof(channels));
}

struct le_iso *le_iso_open(u16 handle, const struct le_iso_config *config)
{
    if (!config || !config->sdu_len || config->sdu_len > LE_ISO_MAX_SDU)
        return NULL;

    for (unsigned int i = 0; i < LE_ISO_MAX_CHANNELS; i++) {
        if (channels[i].used)
            continue;
        memset(&channels[i], 0, sizeof(channels[i]));
        channels[i].used = true;
        channels[i].handle = handle;
        channels[i].config = *config;
        if (!channels[i].config.pdu_len)
            channels[i].config.pdu_len = 251;
        return &channels[i];
    }
    return NULL;
}

int le_iso_start(struct le_iso *iso)
{
    if (!iso)
        return -1;
    iso->running = true;
    iso->next_event_slot = 0;
    return 0;
}

int le_iso_stop(struct le_iso *iso)
{
    if (!iso)
        return -1;
    iso->running = false;
    return 0;
}

/*
 * ISOAL transmit: unframed passes the SDU through, framed prepends the
 * 2-byte segmentation header (SN and segmentation state) and splits at the
 * PDU boundary.
 */
int le_iso_send(struct le_iso *iso, const u8 *sdu, u16 len)
{
    if (!iso || !iso->running || !sdu || len > LE_ISO_MAX_SDU)
        return -1;

    if (iso->config.framing == LE_ISO_UNFRAMED) {
        if (len > iso->config.pdu_len)
            return -2;
        memcpy(iso->tx_buffer, sdu, len);
        iso->tx_len = len;
    } else {
        u16 header = (u16)((iso->sequence & 0x7F) << 2);
        u16 chunk = len > iso->config.pdu_len - 2 ?
                    (u16)(iso->config.pdu_len - 2) : len;
        iso->tx_buffer[0] = (u8)(header & 0xFF);
        iso->tx_buffer[1] = (u8)(header >> 8);
        memcpy(&iso->tx_buffer[2], sdu, chunk);
        iso->tx_len = (u16)(chunk + 2);
        iso->sequence++;
    }

    iso->stats.sdu_sent++;
    return 0;
}

int le_iso_receive(struct le_iso *iso, const u8 *pdu, u16 len)
{
    if (!iso || !iso->running || !pdu)
        return -1;

    if (iso->config.framing == LE_ISO_UNFRAMED) {
        if (len > sizeof(iso->rx_assembly))
            len = sizeof(iso->rx_assembly);
        memcpy(iso->rx_assembly, pdu, len);
        iso->rx_len = len;
        iso->stats.sdu_received++;
        return len;
    }

    if (len < 2)
        return -2;

    u16 header = (u16)(pdu[0] | (pdu[1] << 8));
    u8 seq = (u8)((header >> 2) & 0x7F);
    u8 seg = (u8)(header & 0x03);

    if (seg == 0x03) {
        iso->rx_len = 0;
    } else if (seq != iso->rx_seq) {
        iso->stats.lost++;
    }
    iso->rx_seq = seq;

    u16 available = (u16)(iso->config.sdu_len - iso->rx_len);
    u16 chunk = (u16)(len - 2);
    if (chunk > available)
        chunk = available;

    memcpy(&iso->rx_assembly[iso->rx_len], &pdu[2], chunk);
    iso->rx_len += chunk;

    if (seg == 0x01 || seg == 0x02) {
        iso->stats.sdu_received++;
        iso->stats.reassembled++;
        u16 total = iso->rx_len;
        iso->rx_len = 0;
        return total;
    }
    return 0;
}

void le_iso_tick(u32 slot)
{
    for (unsigned int i = 0; i < LE_ISO_MAX_CHANNELS; i++) {
        struct le_iso *iso = &channels[i];
        if (!iso->used || !iso->running)
            continue;

        /* ISO interval is expressed in microseconds; 625 us per slot. */
        u32 interval_slots = iso->config.interval_us / 625;
        if (!interval_slots)
            interval_slots = 1;

        if (slot < iso->next_event_slot)
            continue;

        if (iso->tx_len) {
            radio_tx(iso->tx_buffer, iso->tx_len);
            iso->tx_len = 0;
            iso->stats.sdu_sent++;
        }

        iso->next_event_slot = slot + interval_slots;
    }
}

const struct le_iso_stats *le_iso_stats(struct le_iso *iso)
{
    return iso ? &iso->stats : 0;
}
