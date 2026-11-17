/*
 * Simulated hardware SBC decode -> TWS both ears.
 *
 * SBC frames are parsed with the real header parser, the simulated HW
 * decoder returns reference PCM (produced from the same SBC stream), and the
 * PCM is relayed through the TWS v3 MEDIA path to the secondary.
 */
#include <bt/tws.h>
#include <bt/l2cap.h>
#include <bt/lmp.h>
#include <bt/avdtp.h>
#include <audio/sbc_hw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFFSET_SAMPLES 441

static struct tws primary, secondary;
static struct l2cap_chan ch_primary, ch_secondary;
static s16 *primary_pcm, *secondary_pcm;
static u32 total_samples, received;

struct l2cap_chan *l2cap_connect(u16 psm,u16 h,u16 m,const struct l2cap_ops*o,void*x)
{ (void)psm;(void)m;(void)o;(void)x; return h==1?&ch_primary:&ch_secondary; }
int l2cap_send(struct l2cap_chan *c,const void*d,u16 l)
{ struct tws*t=(c==&ch_primary)?&secondary:&primary; struct bt_buf*b=bt_buf_alloc(0);
  if(!b){return -1;}
  bt_buf_append(b,d,l); tws_rx(t,b); return 0; }
int l2cap_disconnect(struct l2cap_chan*c){(void)c;return 0;}
struct lmp_link *lmp_link_find(u16 h){(void)h;return NULL;}
int lmp_role_switch(struct lmp_link*l){(void)l;return 0;}
void a2dp_set_relay(struct a2dp*a,void(*f)(const u8*,u16,u32,void*),void*g){(void)a;(void)f;(void)g;}

static void on_media(const u8*d,u16 l,u32 ts,void*a)
{ (void)a; memcpy(&secondary_pcm[ts+OFFSET_SAMPLES],d,l); received++; }
static void on_key(u8 k,u8 s,void*a){(void)k;(void)s;(void)a;}
static void on_bat(u8 p,void*a){(void)p;(void)a;}

static void wav(const char*p,const s16*d,u32 n)
{ FILE*f=fopen(p,"wb"); u32 db=n*2,r=36+db,fm=16,rt=44100,br=rt*2; u16 pc=1,ch=1,al=2,bi=16;
  if(!f){return;}
  fwrite("RIFF",1,4,f);fwrite(&r,4,1,f);fwrite("WAVEfmt ",1,8,f);
  fwrite(&fm,4,1,f);fwrite(&pc,2,1,f);fwrite(&ch,2,1,f);fwrite(&rt,4,1,f);
  fwrite(&br,4,1,f);fwrite(&al,2,1,f);fwrite(&bi,2,1,f);fwrite("data",1,4,f);
  fwrite(&db,4,1,f);fwrite(d,2,n,f);fclose(f); }

int main(int argc,char**argv)
{
    const char *sbc_path=argc>1?argv[1]:"tmp/song.sbc";
    const char *pcm_path=argc>2?argv[2]:"tmp/song.pcm";
    struct tws_callbacks cb={.on_media=on_media,.on_key=on_key,.on_battery=on_bat};
    bt_addr pa={{1,2,3,4,5,6}},pb={{6,5,4,3,2,1}},ph={{0x11,0x22,0x33,0x44,0x55,0x66}};
    FILE *fs=fopen(sbc_path,"rb"); if(!fs){perror("sbc");return 1;}
    fseek(fs,0,SEEK_END); long ssz=ftell(fs); fseek(fs,0,SEEK_SET);
    u8 *sbc=malloc(ssz); if(fread(sbc,1,ssz,fs)!=(size_t)ssz)return 1; fclose(fs);
    FILE *fp=fopen(pcm_path,"rb"); if(!fp){perror("pcm");return 1;}
    fseek(fp,0,SEEK_END); long psz=ftell(fp); fseek(fp,0,SEEK_SET);
    total_samples=(u32)(psz/2);
    primary_pcm=malloc(total_samples*2); secondary_pcm=calloc(total_samples+OFFSET_SAMPLES,2);
    if(fread(primary_pcm,2,total_samples,fp)!=(size_t)total_samples){return 1;}
    fclose(fp);
    sbc_hw_sim_load(primary_pcm,total_samples,44100,1);

    tws_init(&primary,TWS_ROLE_PRIMARY,&cb,NULL);
    tws_init(&secondary,TWS_ROLE_SECONDARY,&cb,NULL);
    tws_connect(&primary,&pb,1); tws_connect(&secondary,&pa,2);
    tws_hello(&primary,&ph,0x0F); tws_sync(&primary,OFFSET_SAMPLES,0);

    u32 off=0, at=0;
    while (off + 4 <= (u32)ssz) {
        struct sbc_frame_info info;
        if (sbc_parse_header(&sbc[off],(u32)ssz-off,&info))
            break;
        s16 pcm[1024];
        int n=sbc_hw_decode(&sbc[off],info.frame_len,pcm,1024);
        if (n<=0) break;
        tws_relay_media(&primary,pcm,(u16)(n*2),at);
        at += (u32)n;
        off += info.frame_len;
    }

    u32 mism=0;
    for(u32 i=0;i<at;i++) if(primary_pcm[i]!=secondary_pcm[i+OFFSET_SAMPLES]) mism++;
    wav("tmp/tws_hw_primary.wav",primary_pcm,at);
    wav("tmp/tws_hw_secondary.wav",&secondary_pcm[OFFSET_SAMPLES],at);
    printf("sbc bytes=%ld frames=%u pcm=%u relayed=%u\n",ssz,sbc_hw_frames_decoded(),at,received);
    printf("hw-sim both ears: %s (%u mismatches)\n",mism?"FAIL":"PASS",mism);
    return mism?1:0;
}
