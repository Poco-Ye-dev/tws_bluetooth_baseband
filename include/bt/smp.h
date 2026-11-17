#ifndef __BT_SMP_H__
#define __BT_SMP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/l2cap.h>

/*
 * Security manager: LE pairing and key distribution.
 *
 * The state machine implements Just Works and Passkey entry.  Keys are handed
 * to a persistent store callback, so bonding survives a reboot without the
 * stack knowing where flash lives.
 */

#define SMP_KEY_LEN 16

enum smp_state {
    SMP_IDLE = 0,
    SMP_WAIT_PAIRING_REQ,
    SMP_WAIT_PAIRING_RSP,
    SMP_WAIT_CONFIRM,
    SMP_WAIT_RANDOM,
    SMP_WAIT_ENCRYPT,
    SMP_PAIRED,
    SMP_FAILED,
};

enum smp_io_cap {
    SMP_IO_DISPLAY_ONLY = 0,
    SMP_IO_DISPLAY_YES_NO,
    SMP_IO_KEYBOARD_ONLY,
    SMP_IO_NO_INPUT_NO_OUTPUT,
    SMP_IO_KEYBOARD_DISPLAY,
};

struct smp_keys {
    u8 ltk[SMP_KEY_LEN];
    u8 irk[SMP_KEY_LEN];
    u8 csrk[SMP_KEY_LEN];
    u16 ediv;
    u8 rand[8];
    bool valid;
};

struct smp_store_ops {
    int (*load)(const bt_addr *peer, struct smp_keys *keys);
    int (*save)(const bt_addr *peer, const struct smp_keys *keys);
    int (*remove)(const bt_addr *peer);
};

struct smp {
    enum smp_state state;
    struct l2cap_chan *chan;
    bt_addr peer;
    enum smp_io_cap io_cap;
    bool mitm;
    bool bonded;
    struct smp_keys keys;
    const struct smp_store_ops *store;
    void (*on_paired)(const bt_addr *peer, bool success, void *arg);
    void *arg;
};

int smp_init(struct smp *smp, const struct smp_store_ops *store);
int smp_pair(struct smp *smp, struct l2cap_chan *chan, enum smp_io_cap io_cap,
             bool mitm);
int smp_cancel(struct smp *smp);
void smp_rx(struct smp *smp, struct bt_buf *buf);
bool smp_bonded(const struct smp *smp);

#endif /* __BT_SMP_H__ */
