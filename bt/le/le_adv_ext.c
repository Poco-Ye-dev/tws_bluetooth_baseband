#include <bt/le_adv_ext.h>
#include <bt/radio.h>
#include <bt/baseband.h>
#include <core/string.h>

static struct le_ext_adv sets[LE_EXT_MAX_SETS];

void le_ext_adv_init(void)
{
    memset(sets, 0, sizeof(sets));
}

struct le_ext_adv *le_ext_adv_open(const struct le_ext_adv_config *config)
{
    if (!config)
        return NULL;

    for (unsigned int i = 0; i < LE_EXT_MAX_SETS; i++) {
        if (sets[i].used)
            continue;
        memset(&sets[i], 0, sizeof(sets[i]));
        sets[i].used = true;
        sets[i].config = *config;
        sets[i].aux_channel = 0;
        return &sets[i];
    }
    return NULL;
}

int le_ext_adv_set_data(struct le_ext_adv *adv, const u8 *data, u16 len)
{
    if (!adv || !data || len > sizeof(adv->data))
        return -1;
    memcpy(adv->data, data, len);
    adv->len = len;
    return 0;
}

int le_ext_adv_enable(struct le_ext_adv *adv, bool enable)
{
    if (!adv)
        return -1;
    adv->enabled = enable;
    adv->next_slot = 0;
    adv->sequence = 0;
    return 0;
}

static void send_primary(struct le_ext_adv *adv, u32 slot)
{
    u8 pdu[64];
    u8 chan = (u8)(adv->config.primary_channel % 3);

    /* ADV_EXT_IND: extended header with ADI and AuxPtr. */
    pdu[0] = 0xAA;
    pdu[1] = 0xD6;
    pdu[2] = 0xBE;
    pdu[3] = 0x89;
    pdu[4] = 0x8E;
    pdu[5] = 0x07;                  /* ADV_EXT_IND */
    pdu[6] = 0x06;                  /* extended header length */
    pdu[7] = 0x01;                  /* ADI present */
    pdu[8] = 0x00;
    pdu[9] = (u8)(adv->sequence & 0xFF);
    pdu[10] = 0x03;                 /* AuxPtr */
    pdu[11] = (u8)(adv->aux_channel);
    pdu[12] = (u8)((slot + 3) & 0xFF);

    radio_tune((u16)bb_le_adv_channel(chan));
    radio_tx(pdu, 13);

    adv->aux_channel = (u8)((adv->aux_channel + 1) % 37);
    adv->sequence++;
}

static void send_aux(struct le_ext_adv *adv)
{
    u8 pdu[LE_EXT_ADV_MAX + 16];

    pdu[0] = 0xAA;
    pdu[1] = 0xD6;
    pdu[2] = 0xBE;
    pdu[3] = 0x89;
    pdu[4] = 0x8E;
    pdu[5] = 0x07;
    pdu[6] = (u8)(2 + adv->len);
    pdu[7] = 0x00;
    pdu[8] = 0x00;
    memcpy(&pdu[9], adv->data, adv->len);

    radio_tune(adv->aux_channel);
    radio_tx(pdu, (u16)(9 + adv->len));
}

void le_ext_adv_tick(u32 slot)
{
    for (unsigned int i = 0; i < LE_EXT_MAX_SETS; i++) {
        struct le_ext_adv *adv = &sets[i];
        if (!adv->used || !adv->enabled)
            continue;
        if (slot < adv->next_slot)
            continue;

        send_primary(adv, slot);
        send_aux(adv);

        if (adv->config.periodic && adv->config.periodic_interval)
            adv->next_slot = slot + adv->config.periodic_interval;
        else
            adv->next_slot = slot + (adv->config.interval ? adv->config.interval : 0x20);
    }
}
