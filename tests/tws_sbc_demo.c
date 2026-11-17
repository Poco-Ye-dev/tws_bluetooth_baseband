/*
 * SBC song -> TWS both ears (host demo).
 *
 * Input is a raw S16 PCM file produced by decoding an SBC stream (the host
 * pipeline uses ffmpeg for SBC decode; TWS distribution and sync are src_ai).
 * The primary pushes PCM frames through the TWS v3 MEDIA path, the secondary
 * aligns by the SYNC offset, and both sides write a WAV for listening.
 */
#include <bt/tws.h>
#include <bt/l2cap.h>
#include <bt/lmp.h>
#include <bt/avdtp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RATE 44100
#define FRAME_SAMPLES 240
#define OFFSET_SAMPLES 441          /* 10 ms */

static struct tws primary, secondary;
static struct l2cap_chan ch_primary, ch_secondary;
static s16 *primary_pcm;
static s16 *secondary_pcm;
static u32 total_samples;
static u32 received_frames;

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

    if (at + samples > total_samples + OFFSET_SAMPLES)
        return;
    memcpy(&secondary_pcm[at], data, len);
    received_frames++;
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

int main(int argc, char **argv)
{
    const char *pcm_path = argc > 1 ? argv[1] : "tmp/song.pcm";
    struct tws_callbacks cb = { .on_media = on_media, .on_key = on_key,
                                .on_battery = on_bat };
    bt_addr pa = {{1,2,3,4,5,6}}, pb = {{6,5,4,3,2,1}};
    bt_addr phone = {{0x11,0x22,0x33,0x44,0x55,0x66}};

    FILE *f = fopen(pcm_path, "rb");
    if (!f) { perror("open pcm"); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 2) { fclose(f); return 1; }

    total_samples = (u32)(size / 2);
    primary_pcm = calloc(total_samples + OFFSET_SAMPLES, 2);
    secondary_pcm = calloc(total_samples + OFFSET_SAMPLES, 2);
    if (!primary_pcm || !secondary_pcm) return 1;
    if (fread(primary_pcm, 2, total_samples, f) != total_samples) return 1;
    fclose(f);

    tws_init(&primary, TWS_ROLE_PRIMARY, &cb, NULL);
    tws_init(&secondary, TWS_ROLE_SECONDARY, &cb, NULL);
    tws_connect(&primary, &pb, 1);
    tws_connect(&secondary, &pa, 2);
    tws_hello(&primary, &phone, 0x0F);
    tws_sync(&primary, OFFSET_SAMPLES, 0);

    for (u32 at = 0; at < total_samples; at += FRAME_SAMPLES) {
        u32 n = total_samples - at;
        if (n > FRAME_SAMPLES) n = FRAME_SAMPLES;
        tws_relay_media(&primary, &primary_pcm[at], (u16)(n * 2), at);
    }

    u32 mismatches = 0;
    for (u32 i = 0; i < total_samples; i++) {
        if (primary_pcm[i] != secondary_pcm[i + OFFSET_SAMPLES])
            mismatches++;
    }

    write_wav("tmp/tws_sbc_primary.wav", primary_pcm, total_samples);
    write_wav("tmp/tws_sbc_secondary.wav", &secondary_pcm[OFFSET_SAMPLES], total_samples);

    printf("samples: %u, frames: %u, offset: %d samples\n",
           total_samples, received_frames, OFFSET_SAMPLES);
    printf("both ears: %s (%u mismatches)\n",
           mismatches ? "FAIL" : "PASS", mismatches);
    printf("wav: tmp/tws_sbc_primary.wav tmp/tws_sbc_secondary.wav\n");
    return mismatches ? 1 : 0;
}
