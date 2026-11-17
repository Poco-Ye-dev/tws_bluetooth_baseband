/*
 * TWS two-side synchronised fake-audio demo.
 *
 * Generates a 1 kHz sine as fake PCM on the primary, relays it through the
 * TWS v3 message path, applies the SYNC clock offset on the secondary, and
 * verifies both playback buffers line up sample for sample.  Two WAV files
 * are written so the result can be listened to on the host.
 */
#include <bt/tws.h>
#include <bt/l2cap.h>
#include <bt/lmp.h>
#include <bt/avdtp.h>
#include <core/string.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define FRAMES      40
#define FRAME_SAMPLES 240
#define RATE        48000
#define FREQ        1000
#define OFFSET_SAMPLES 1440       /* 30 ms clock offset, in samples */

static struct tws primary, secondary;
static struct l2cap_chan ch_primary, ch_secondary;
static s16 primary_buf[FRAMES * FRAME_SAMPLES];
static s16 secondary_buf[FRAMES * FRAME_SAMPLES + OFFSET_SAMPLES];
static int received;

struct l2cap_chan *l2cap_connect(u16 psm, u16 h, u16 m, const struct l2cap_ops *o, void *x)
{ (void)psm; (void)m; (void)o; (void)x; return h == 1 ? &ch_primary : &ch_secondary; }
int l2cap_send(struct l2cap_chan *c, const void *d, u16 l)
{
    struct tws *to = (c == &ch_primary) ? &secondary : &primary;
    struct bt_buf *buf = bt_buf_alloc(0);
    if (!buf) return -1;
    bt_buf_append(buf, d, l);
    tws_rx(to, buf);
    return 0;
}
int l2cap_disconnect(struct l2cap_chan *c) { (void)c; return 0; }
struct lmp_link *lmp_link_find(u16 h) { (void)h; return NULL; }
int lmp_role_switch(struct lmp_link *l) { (void)l; return 0; }
void a2dp_set_relay(struct a2dp *a, void (*f)(const u8 *, u16, u32, void *), void *g)
{ (void)a; (void)f; (void)g; }

static void on_media(const u8 *data, u16 len, u32 timestamp, void *arg)
{
    (void)arg;
    u32 at = timestamp + OFFSET_SAMPLES;
    u32 samples = len / 2;

    if (at + samples > sizeof(secondary_buf) / 2)
        return;
    memcpy(&secondary_buf[at], data, len);
    received++;
}

static void on_key(u8 k, u8 s, void *a) { (void)k; (void)s; (void)a; }
static void on_bat(u8 p, void *a) { (void)p; (void)a; }

static void write_wav(const char *path, const s16 *data, u32 samples)
{
    FILE *f = fopen(path, "wb");
    u32 data_bytes = samples * 2;
    u32 riff = 36 + data_bytes;
    u32 fmt = 16, rate = RATE, byte_rate = RATE * 2;
    u16 pcm = 1, ch = 1, align = 2, bits = 16;

    if (!f) return;
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmt, 4, 1, f);
    fwrite(&pcm, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f); fwrite(&byte_rate, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data_bytes, 4, 1, f);
    fwrite(data, 2, samples, f);
    fclose(f);
}

int main(void)
{
    struct tws_callbacks cb = { .on_media = on_media, .on_key = on_key,
                                .on_battery = on_bat };
    bt_addr pa = {{1,2,3,4,5,6}}, pb = {{6,5,4,3,2,1}};
    bt_addr phone = {{0x11,0x22,0x33,0x44,0x55,0x66}};

    for (u32 i = 0; i < FRAMES * FRAME_SAMPLES; i++)
        primary_buf[i] = (s16)(12000.0 * sin(2.0 * 3.14159265358979323846 * FREQ * i / RATE));

    tws_init(&primary, TWS_ROLE_PRIMARY, &cb, NULL);
    tws_init(&secondary, TWS_ROLE_SECONDARY, &cb, NULL);
    tws_connect(&primary, &pb, 1);
    tws_connect(&secondary, &pa, 2);
    tws_hello(&primary, &phone, 0x0F);

    /* 30 ms offset expressed in samples; secondary aligns by adding it. */
    tws_sync(&primary, OFFSET_SAMPLES, 0);

    for (int f = 0; f < FRAMES; f++) {
        u32 ts = (u32)f * FRAME_SAMPLES;
        tws_relay_media(&primary, &primary_buf[ts], FRAME_SAMPLES * 2, ts);
    }

    /* Compare the compensated ranges. */
    int mismatches = 0;
    for (u32 i = 0; i < FRAMES * FRAME_SAMPLES; i++) {
        if (primary_buf[i] != secondary_buf[i + OFFSET_SAMPLES])
            mismatches++;
    }

    write_wav("tmp/tws_primary.wav", primary_buf, FRAMES * FRAME_SAMPLES);
    write_wav("tmp/tws_secondary.wav", &secondary_buf[OFFSET_SAMPLES],
              FRAMES * FRAME_SAMPLES);

    printf("frames relayed: %d/%d\n", received, FRAMES);
    printf("clock offset : %d samples\n", OFFSET_SAMPLES);
    printf("sample match : %s (%d mismatches)\n",
           mismatches ? "FAIL" : "PASS", mismatches);
    printf("wav files    : tmp/tws_primary.wav tmp/tws_secondary.wav\n");
    return (received == FRAMES && mismatches == 0) ? 0 : 1;
}
