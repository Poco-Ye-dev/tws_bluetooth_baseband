#ifndef __BT_LMP_H__
#define __BT_LMP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/edr.h>
#include <crypto/e0.h>

/*
 * Link Manager Protocol.
 *
 * LMP procedures are serialised per link: one procedure may be outstanding
 * at a time and is completed by LMP_ACCEPTED or LMP_NOT_ACCEPTED.  This is
 * the state machine that owns authentication, encryption, role switch,
 * sniff, eSCO and power control requests.
 */

enum lmp_opcode {
    LMP_NAME_REQ          = 0x01,
    LMP_NAME_RES          = 0x02,
    LMP_ACCEPTED          = 0x03,
    LMP_NOT_ACCEPTED      = 0x04,
    LMP_CLKOFFSET_REQ     = 0x05,
    LMP_CLKOFFSET_RES     = 0x06,
    LMP_DETACH            = 0x07,
    LMP_AU_RAND           = 0x0B,
    LMP_SRES              = 0x0C,
    LMP_FEATURES_REQ      = 0x1B,
    LMP_FEATURES_RES      = 0x1C,
    LMP_VERSION_REQ       = 0x1D,
    LMP_VERSION_RES       = 0x1E,
    LMP_ENCRYPTION_MODE_REQ = 0x2F,
    LMP_ENCRYPTION_KEY_SIZE_REQ = 0x30,
    LMP_START_ENCRYPTION_REQ = 0x31,
    LMP_STOP_ENCRYPTION_REQ  = 0x32,
    LMP_SWITCH_REQ        = 0x21,
    LMP_HOLD              = 0x22,
    LMP_SNIFF             = 0x23,
    LMP_UNSNIFF           = 0x24,
    LMP_MAX_SLOT_REQ      = 0x26,
    LMP_MAX_SLOT          = 0x27,
    LMP_ESCO_LINK_REQ     = 0x28,
    LMP_ESCO_LINK_RES     = 0x29,
    LMP_REMOVE_ESCO_LINK_REQ = 0x2A,
    LMP_MAX_POWER         = 0x2B,
    LMP_MIN_POWER         = 0x2C,
    LMP_POWER_INCREMENT_REQ = 0x2D,
    LMP_POWER_INCREMENT_RES = 0x2E,
};

enum lmp_state {
    LMP_IDLE = 0,
    LMP_WAIT_ACCEPT,
    LMP_WAIT_AUTH,
    LMP_WAIT_ENCRYPT,
    LMP_WAIT_SWITCH,
    LMP_WAIT_SNIFF,
    LMP_WAIT_ESCO,
};

struct lmp_link;

struct lmp_ops {
    int (*send)(struct lmp_link *link, u8 opcode, const void *data, u16 len);
    void (*authenticated)(struct lmp_link *link, bool ok);
    void (*encrypted)(struct lmp_link *link, bool on);
    void (*role_changed)(struct lmp_link *link, u8 role);
    void (*sniff)(struct lmp_link *link, bool on);
    void (*esco)(struct lmp_link *link, const struct edr_esco_params *params);
    void (*detached)(struct lmp_link *link, u8 reason);
};

struct lmp_link {
    bool used;
    u16 handle;
    bt_addr peer;
    u8 features[8];
    u8 version;
    u16 manufacturer;
    u16 subversion;
    u8 name[248];
    u8 name_len;
    u8 role;
    bool authenticated;
    bool encrypted;
    enum lmp_state state;
    u8 pending_opcode;
    u8 tx_seq;
    u8 retries;
    u8 link_key[16];
    u8 rand[16];
    bool key_valid;
    struct e0_ctx e0;
};

void lmp_init(const struct lmp_ops *ops);
struct lmp_link *lmp_link_open(u16 handle, const bt_addr *peer, u8 role);
void lmp_link_close(u16 handle);
struct lmp_link *lmp_link_find(u16 handle);

int lmp_send_features(struct lmp_link *link);
int lmp_send_version(struct lmp_link *link);
int lmp_send_name(struct lmp_link *link);
int lmp_authenticate(struct lmp_link *link);
int lmp_start_encryption(struct lmp_link *link);
int lmp_stop_encryption(struct lmp_link *link);
int lmp_role_switch(struct lmp_link *link);
int lmp_sniff(struct lmp_link *link, u16 interval, u16 attempt, u16 timeout);
int lmp_unsniff(struct lmp_link *link);
int lmp_esco_request(struct lmp_link *link, const struct edr_esco_params *params);
int lmp_detach(struct lmp_link *link, u8 reason);
int lmp_max_slot(struct lmp_link *link, u8 slots);
int lmp_power_increment(struct lmp_link *link, u8 step);
int lmp_set_link_key(struct lmp_link *link, const u8 key[16]);
int lmp_encrypt(struct lmp_link *link, const u8 *in, u8 *out, u32 len);
int lmp_decrypt(struct lmp_link *link, const u8 *in, u8 *out, u32 len);

void lmp_rx(struct lmp_link *link, u8 opcode, const u8 *data, u16 len);

#endif /* __BT_LMP_H__ */
