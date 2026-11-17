#ifndef __BT_AVDTP_H__
#define __BT_AVDTP_H__

#include <core/types.h>
#include <bt/l2cap.h>
#include <bt/buf.h>

/*
 * AVDTP/A2DP signaling and basic media channel.
 */

#define AVDTP_PSM 0x0019

enum avdtp_msg {
    AVDTP_DISCOVER = 0x01,
    AVDTP_GET_CAPABILITIES = 0x02,
    AVDTP_SET_CONFIGURATION = 0x03,
    AVDTP_GET_CONFIGURATION = 0x04,
    AVDTP_RECONFIGURE = 0x05,
    AVDTP_OPEN = 0x06,
    AVDTP_START = 0x07,
    AVDTP_CLOSE = 0x08,
    AVDTP_SUSPEND = 0x09,
    AVDTP_ABORT = 0x0A,
};

enum avdtp_state {
    AVDTP_ST_IDLE = 0,
    AVDTP_ST_CONFIGURED,
    AVDTP_ST_OPEN,
    AVDTP_ST_STREAMING,
};

struct a2dp {
    struct l2cap_chan *sig;
    struct l2cap_chan *media;
    enum avdtp_state state;
    u8 seid;
    u8 codec;
    u8 config[16];
    u16 handle;
    u8 transaction;
    void (*on_stream)(bool streaming, void *arg);
    void (*relay)(const u8 *data, u16 len, u32 timestamp, void *arg);
    void *arg;
    void *relay_arg;
};

int a2dp_init(struct a2dp *a2dp, void (*on_stream)(bool, void *), void *arg);
int a2dp_connect(struct a2dp *a2dp, u16 handle);
int a2dp_start(struct a2dp *a2dp);
int a2dp_stop(struct a2dp *a2dp);
int a2dp_send_media(struct a2dp *a2dp, const void *data, u16 len);
void a2dp_set_relay(struct a2dp *a2dp,
                    void (*relay)(const u8 *, u16, u32, void *), void *arg);
void a2dp_rx(struct a2dp *a2dp, struct bt_buf *buf);
void a2dp_close(struct a2dp *a2dp);

#endif /* __BT_AVDTP_H__ */
