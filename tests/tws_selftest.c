/*
 * Host self test for the TWS v3 protocol state machine.
 *
 * It links tws.c + buf.c and provides minimal L2CAP/LMP/A2DP stubs so the
 * protocol can run without radio hardware.  The two instances talk to each
 * other through the stubbed L2CAP channel, which is exactly the message path
 * used on target once the real channels are open.
 */

#include <bt/tws.h>
#include <bt/l2cap.h>
#include <bt/lmp.h>
#include <bt/avdtp.h>
#include <core/printk.h>
#include <core/string.h>
#include <stdio.h>
#include <string.h>

static struct tws primary;
static struct tws secondary;
static struct l2cap_chan ch_primary;
static struct l2cap_chan ch_secondary;

static int failures;
static int checks;

static void expect(bool cond, const char *what)
{
    checks++;
    if (cond) {
        printf("PASS  %s\n", what);
    } else {
        printf("FAIL  %s\n", what);
        failures++;
    }
}

/* ---- L2CAP stubs ------------------------------------------------------ */

struct l2cap_chan *l2cap_connect(u16 psm, u16 handle, u16 mtu,
                                 const struct l2cap_ops *ops, void *arg)
{
    (void)psm;
    (void)mtu;
    (void)ops;
    (void)arg;
    return handle == 1 ? &ch_primary : &ch_secondary;
}

int l2cap_send(struct l2cap_chan *chan, const void *data, u16 len)
{
    struct tws *to;

    if (chan == &ch_primary)
        to = &secondary;
    else if (chan == &ch_secondary)
        to = &primary;
    else
        return -1;

    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf)
        return -2;
    bt_buf_append(buf, data, len);
    tws_rx(to, buf);
    return 0;
}

int l2cap_disconnect(struct l2cap_chan *chan)
{
    (void)chan;
    return 0;
}

/* ---- LMP / A2DP stubs ------------------------------------------------- */

struct lmp_link *lmp_link_find(u16 handle)
{
    (void)handle;
    return NULL;
}

int lmp_role_switch(struct lmp_link *link)
{
    (void)link;
    return 0;
}

void a2dp_set_relay(struct a2dp *a2dp,
                    void (*relay)(const u8 *, u16, u32, void *), void *arg)
{
    (void)a2dp;
    (void)relay;
    (void)arg;
}

/* ---- callbacks -------------------------------------------------------- */

static u8 last_key, last_state;
static u8 last_battery;
static u16 last_media_len;
static u32 last_media_ts;

static void on_media(const u8 *data, u16 len, u32 timestamp, void *arg)
{
    (void)data;
    (void)arg;
    last_media_len = len;
    last_media_ts = timestamp;
}

static void on_key(u8 key, u8 state, void *arg)
{
    (void)arg;
    last_key = key;
    last_state = state;
}

static void on_battery(u8 percent, void *arg)
{
    (void)arg;
    last_battery = percent;
}

int main(void)
{
    const struct tws_callbacks cb_primary = {
        .on_media = on_media,
        .on_key = on_key,
        .on_battery = on_battery,
    };
    const struct tws_callbacks cb_secondary = {
        .on_media = on_media,
        .on_key = on_key,
        .on_battery = on_battery,
    };

    bt_addr peer_a = { .b = { 1, 2, 3, 4, 5, 6 } };
    bt_addr peer_b = { .b = { 6, 5, 4, 3, 2, 1 } };
    bt_addr phone = { .b = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };

    printf("TWS v3 self test\n");

    tws_init(&primary, TWS_ROLE_PRIMARY, &cb_primary, NULL);
    tws_init(&secondary, TWS_ROLE_SECONDARY, &cb_secondary, NULL);

    expect(tws_connect(&primary, &peer_b, 1) == 0, "primary channel open");
    expect(tws_connect(&secondary, &peer_a, 2) == 0, "secondary channel open");

    tws_hello(&primary, &phone, 0x0F);
    expect(secondary.sync_state == TWS_SYNC_READY, "HELLO reaches secondary");
    expect(memcmp(secondary.phone.b, phone.b, 6) == 0, "phone address shared");

    tws_sync(&primary, 0x1234, 0x5678);
    expect(secondary.clock_offset == 0x1234, "clock offset synced");
    expect(secondary.anchor == 0x5678, "anchor synced");

    tws_relay_media(&primary, (const u8 *)"sbc-frame", 9, 1000);
    expect(last_media_len == 9, "media relayed to secondary");
    expect(last_media_ts == 1000, "media timestamp preserved");

    tws_send_key(&primary, 0x44, 0);
    expect(last_key == 0x44 && last_state == 0, "key relayed");

    tws_send_battery(&primary, 77);
    expect(last_battery == 77, "battery relayed");

    tws_request_role_switch(&primary);
    expect(primary.role == TWS_ROLE_SECONDARY, "primary became secondary");
    expect(secondary.role == TWS_ROLE_PRIMARY, "secondary became primary");

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
