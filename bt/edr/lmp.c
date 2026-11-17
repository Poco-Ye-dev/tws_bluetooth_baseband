#include <bt/lmp.h>
#include <core/string.h>
#include <core/errno.h>
#include <crypto/e0.h>

#define LMP_MAX_LINKS 8

static struct lmp_link links[LMP_MAX_LINKS];
static struct lmp_ops lmp_ops;

void lmp_init(const struct lmp_ops *ops)
{
    memset(links, 0, sizeof(links));
    if (ops)
        lmp_ops = *ops;
}

struct lmp_link *lmp_link_find(u16 handle)
{
    for (unsigned int i = 0; i < LMP_MAX_LINKS; i++) {
        if (links[i].used && links[i].handle == handle)
            return &links[i];
    }
    return NULL;
}

struct lmp_link *lmp_link_open(u16 handle, const bt_addr *peer, u8 role)
{
    for (unsigned int i = 0; i < LMP_MAX_LINKS; i++) {
        struct lmp_link *link = &links[i];
        if (link->used)
            continue;

        memset(link, 0, sizeof(*link));
        link->used = true;
        link->handle = handle;
        link->peer = *peer;
        link->role = role;
        link->state = LMP_IDLE;
        return link;
    }
    return NULL;
}

void lmp_link_close(u16 handle)
{
    struct lmp_link *link = lmp_link_find(handle);
    if (link)
        memset(link, 0, sizeof(*link));
}

static int send(struct lmp_link *link, u8 opcode, const void *data, u16 len)
{
    if (!link || !lmp_ops.send)
        return -1;
    return lmp_ops.send(link, opcode, data, len);
}

static int send_accept(struct lmp_link *link, u8 opcode)
{
    return send(link, LMP_ACCEPTED, &opcode, 1);
}

static int send_reject(struct lmp_link *link, u8 opcode, u8 reason)
{
    u8 payload[2] = { opcode, reason };
    return send(link, LMP_NOT_ACCEPTED, payload, 2);
}

static int start_procedure(struct lmp_link *link, u8 opcode,
                           const void *data, u16 len, enum lmp_state state)
{
    int ret = send(link, opcode, data, len);
    if (!ret) {
        link->state = state;
        link->pending_opcode = opcode;
        link->retries = 0;
    }
    return ret;
}

int lmp_send_features(struct lmp_link *link)
{
    u8 features[8] = { 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00 };
    return start_procedure(link, LMP_FEATURES_REQ, features, 8, LMP_IDLE);
}

int lmp_send_version(struct lmp_link *link)
{
    u8 version[5] = { 0x0C, 0x00, 0x00, 0x00, 0x00 }; /* BT 5.0 */
    return start_procedure(link, LMP_VERSION_REQ, version, 5, LMP_IDLE);
}

int lmp_send_name(struct lmp_link *link)
{
    if (!link)
        return -1;
    return send(link, LMP_NAME_REQ, NULL, 0);
}

int lmp_authenticate(struct lmp_link *link)
{
    u8 rand[16];

    if (!link)
        return -1;
    memset(rand, 0x5A, sizeof(rand));
    memcpy(link->rand, rand, sizeof(rand));
    return start_procedure(link, LMP_AU_RAND, rand, 16, LMP_WAIT_AUTH);
}

int lmp_start_encryption(struct lmp_link *link)
{
    u8 param = 16;

    if (!link)
        return -1;
    return start_procedure(link, LMP_START_ENCRYPTION_REQ, &param, 1,
                           LMP_WAIT_ENCRYPT);
}

int lmp_stop_encryption(struct lmp_link *link)
{
    if (!link)
        return -1;
    link->encrypted = false;
    return send(link, LMP_STOP_ENCRYPTION_REQ, NULL, 0);
}

int lmp_role_switch(struct lmp_link *link)
{
    if (!link)
        return -1;
    return start_procedure(link, LMP_SWITCH_REQ, NULL, 0, LMP_WAIT_SWITCH);
}

int lmp_sniff(struct lmp_link *link, u16 interval, u16 attempt, u16 timeout)
{
    u8 payload[10];

    if (!link)
        return -1;

    payload[0] = (u8)(interval & 0xFF);
    payload[1] = (u8)(interval >> 8);
    payload[2] = (u8)(attempt & 0xFF);
    payload[3] = (u8)(attempt >> 8);
    payload[4] = (u8)(timeout & 0xFF);
    payload[5] = (u8)(timeout >> 8);
    memset(&payload[6], 0, 4);
    return start_procedure(link, LMP_SNIFF, payload, 10, LMP_WAIT_SNIFF);
}

int lmp_unsniff(struct lmp_link *link)
{
    if (!link)
        return -1;
    link->state = LMP_IDLE;
    return send(link, LMP_UNSNIFF, NULL, 0);
}

int lmp_esco_request(struct lmp_link *link, const struct edr_esco_params *params)
{
    u8 payload[7];

    if (!link || !params)
        return -1;

    payload[0] = (u8)(params->te & 0xFF);
    payload[1] = (u8)(params->we & 0xFF);
    payload[2] = (u8)(params->interval & 0xFF);
    payload[3] = (u8)(params->interval >> 8);
    payload[4] = (u8)params->retries;
    payload[5] = (u8)params->packet;
    payload[6] = 0;
    return start_procedure(link, LMP_ESCO_LINK_REQ, payload, 7, LMP_WAIT_ESCO);
}

int lmp_set_link_key(struct lmp_link *link, const u8 key[16])
{
    if (!link || !key)
        return -1;
    memcpy(link->link_key, key, 16);
    link->key_valid = true;
    return 0;
}

int lmp_encrypt(struct lmp_link *link, const u8 *in, u8 *out, u32 len)
{
    if (!link || !link->key_valid || !link->encrypted)
        return -1;
    e0_crypt(&link->e0, in, out, len);
    return 0;
}

int lmp_decrypt(struct lmp_link *link, const u8 *in, u8 *out, u32 len)
{
    return lmp_encrypt(link, in, out, len);
}

int lmp_detach(struct lmp_link *link, u8 reason)
{
    if (!link)
        return -1;
    return send(link, LMP_DETACH, &reason, 1);
}

int lmp_max_slot(struct lmp_link *link, u8 slots)
{
    if (!link)
        return -1;
    return start_procedure(link, LMP_MAX_SLOT_REQ, &slots, 1, LMP_IDLE);
}

int lmp_power_increment(struct lmp_link *link, u8 step)
{
    if (!link)
        return -1;
    return start_procedure(link, LMP_POWER_INCREMENT_REQ, &step, 1, LMP_IDLE);
}

static void handle_accept(struct lmp_link *link, const u8 *data, u16 len)
{
    u8 opcode = len ? data[0] : link->pending_opcode;

    switch (opcode) {
    case LMP_SWITCH_REQ:
        link->role = link->role ? 0 : 1;
        if (lmp_ops.role_changed)
            lmp_ops.role_changed(link, link->role);
        break;
    case LMP_SNIFF:
        if (lmp_ops.sniff)
            lmp_ops.sniff(link, true);
        break;
    case LMP_START_ENCRYPTION_REQ:
        link->encrypted = link->key_valid;
        if (link->key_valid)
            e0_init(&link->e0, link->link_key, link->rand, (const u8[]){0, 0, 0, 0});
        if (lmp_ops.encrypted)
            lmp_ops.encrypted(link, true);
        break;
    case LMP_ESCO_LINK_REQ:
        if (lmp_ops.esco) {
            struct edr_esco_params params = { 0 };
            lmp_ops.esco(link, &params);
        }
        break;
    default:
        break;
    }
    link->state = LMP_IDLE;
}

static void handle_reject(struct lmp_link *link, const u8 *data, u16 len)
{
    (void)len;
    if (!link)
        return;
    if (data && data[0] == LMP_START_ENCRYPTION_REQ && lmp_ops.encrypted)
        lmp_ops.encrypted(link, false);
    link->state = LMP_IDLE;
}

void lmp_rx(struct lmp_link *link, u8 opcode, const u8 *data, u16 len)
{
    if (!link)
        return;

    switch (opcode) {
    case LMP_ACCEPTED:
        handle_accept(link, data, len);
        break;
    case LMP_NOT_ACCEPTED:
        handle_reject(link, data, len);
        break;
    case LMP_FEATURES_REQ:
        if (len >= 8)
            memcpy(link->features, data, 8);
        send(link, LMP_FEATURES_RES, link->features, 8);
        break;
    case LMP_FEATURES_RES:
        if (len >= 8)
            memcpy(link->features, data, 8);
        break;
    case LMP_VERSION_REQ:
        if (len >= 5) {
            link->version = data[0];
            link->manufacturer = (u16)(data[1] | (data[2] << 8));
        }
        send(link, LMP_VERSION_RES, data, len > 5 ? 5 : len);
        break;
    case LMP_VERSION_RES:
        if (len >= 5) {
            link->version = data[0];
            link->manufacturer = (u16)(data[1] | (data[2] << 8));
        }
        break;
    case LMP_NAME_REQ:
        send(link, LMP_NAME_RES, link->name, link->name_len);
        break;
    case LMP_NAME_RES:
        if (len > sizeof(link->name))
            len = sizeof(link->name);
        memcpy(link->name, data, len);
        link->name_len = (u8)len;
        break;
    case LMP_AU_RAND:
        send(link, LMP_SRES, link->features, 4);
        break;
    case LMP_SRES:
        link->authenticated = true;
        if (lmp_ops.authenticated)
            lmp_ops.authenticated(link, true);
        break;
    case LMP_START_ENCRYPTION_REQ:
        send_accept(link, LMP_START_ENCRYPTION_REQ);
        link->encrypted = link->key_valid;
        if (!link->key_valid)
            break;
        e0_init(&link->e0, link->link_key, link->rand, (const u8[]){0, 0, 0, 0});
        if (lmp_ops.encrypted)
            lmp_ops.encrypted(link, true);
        break;
    case LMP_STOP_ENCRYPTION_REQ:
        send_accept(link, LMP_STOP_ENCRYPTION_REQ);
        link->encrypted = false;
        if (lmp_ops.encrypted)
            lmp_ops.encrypted(link, false);
        break;
    case LMP_SWITCH_REQ:
        send_accept(link, LMP_SWITCH_REQ);
        link->role = link->role ? 0 : 1;
        if (lmp_ops.role_changed)
            lmp_ops.role_changed(link, link->role);
        break;
    case LMP_SNIFF:
        send_accept(link, LMP_SNIFF);
        if (lmp_ops.sniff)
            lmp_ops.sniff(link, true);
        break;
    case LMP_UNSNIFF:
        send_accept(link, LMP_UNSNIFF);
        if (lmp_ops.sniff)
            lmp_ops.sniff(link, false);
        break;
    case LMP_ESCO_LINK_REQ:
        send(link, LMP_ESCO_LINK_RES, data, len);
        break;
    case LMP_MAX_SLOT_REQ:
        send(link, LMP_MAX_SLOT, data, len);
        break;
    case LMP_POWER_INCREMENT_REQ:
        send(link, LMP_POWER_INCREMENT_RES, data, len);
        break;
    case LMP_DETACH:
        link->used = false;
        if (lmp_ops.detached)
            lmp_ops.detached(link, len ? data[0] : 0x13);
        break;
    case LMP_CLKOFFSET_REQ: {
        u8 offset[4] = { 0, 0, 0, 0 };
        send(link, LMP_CLKOFFSET_RES, offset, 4);
        break;
    }
    default:
        send_reject(link, opcode, 0x07);
        break;
    }
}
