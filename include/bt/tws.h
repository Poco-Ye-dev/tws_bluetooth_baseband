#ifndef __BT_TWS_H__
#define __BT_TWS_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/l2cap.h>
#include <bt/buf.h>

/*
 * True Wireless Stereo protocol (v3).
 *
 * The primary earbud owns the phone link and relays media/keys to the
 * secondary over a proprietary L2CAP channel.  Clock/anchor sync keeps both
 * sides on the same audio timeline; role switch is a synchronous handover.
 */

#define TWS_PSM 0x0080
#define TWS_MEDIA_MAX 1024

enum tws_role {
    TWS_ROLE_PRIMARY = 0,
    TWS_ROLE_SECONDARY,
};

enum tws_msg {
    TWS_MSG_HELLO = 1,
    TWS_MSG_SYNC,
    TWS_MSG_MEDIA,
    TWS_MSG_KEY,
    TWS_MSG_BATTERY,
    TWS_MSG_ROLE_REQ,
    TWS_MSG_ROLE_ACK,
    TWS_MSG_PING,
    TWS_MSG_MEDIA_UP,
};

enum tws_sync_state {
    TWS_SYNC_IDLE = 0,
    TWS_SYNC_WAIT_HELLO,
    TWS_SYNC_READY,
};

struct tws_stats {
    u32 tx_msgs;
    u32 rx_msgs;
    u32 relayed;
    u32 dropped;
    u32 resyncs;
};

struct tws_callbacks {
    void (*on_media)(const u8 *data, u16 len, u32 timestamp, void *arg);
    void (*on_media_up)(const u8 *data, u16 len, u32 timestamp, void *arg);
    void (*on_key)(u8 key, u8 state, void *arg);
    void (*on_role)(enum tws_role role, void *arg);
    void (*on_battery)(u8 percent, void *arg);
    void (*on_link)(bool connected, void *arg);
};

struct tws {
    enum tws_role role;
    enum tws_sync_state sync_state;
    struct l2cap_chan *chan;
    u16 handle;
    u8 pending_role;
    bt_addr peer;
    bt_addr phone;
    u16 seq;
    u16 media_seq;
    u16 up_seq;
    u32 clock_offset;
    u32 anchor;
    u8 battery;
    struct tws_stats stats;
    struct tws_callbacks cb;
    void *arg;
};

int tws_init(struct tws *tws, enum tws_role role,
             const struct tws_callbacks *cb, void *arg);
int tws_connect(struct tws *tws, const bt_addr *peer, u16 handle);
struct a2dp;
void tws_attach_a2dp(struct tws *tws, struct a2dp *a2dp);
int tws_send(struct tws *tws, u8 type, const void *data, u16 len);
int tws_hello(struct tws *tws, const bt_addr *phone, u8 features);
int tws_sync(struct tws *tws, u32 clock_offset, u32 anchor);
int tws_relay_media(struct tws *tws, const void *data, u16 len, u32 timestamp);
int tws_relay_media_up(struct tws *tws, const void *data, u16 len, u32 timestamp);
int tws_send_key(struct tws *tws, u8 key, u8 state);
int tws_send_battery(struct tws *tws, u8 percent);
int tws_request_role_switch(struct tws *tws);
void tws_rx(struct tws *tws, struct bt_buf *buf);
void tws_close(struct tws *tws);

#endif /* __BT_TWS_H__ */
