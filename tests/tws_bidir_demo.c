/* Bidirectional fake-audio over TWS: primary->secondary and secondary->primary. */
#include <bt/tws.h>
#include <bt/l2cap.h>
#include <bt/lmp.h>
#include <bt/avdtp.h>
#include <stdio.h>
#include <string.h>

#define FRAMES 32
#define N 240
#define DOWN_OFF 441
#define UP_OFF 220

static struct tws a, b; static struct l2cap_chan ca, cb;
static s16 down[N*FRAMES], up[N*FRAMES];
static s16 b_down[N*FRAMES + DOWN_OFF], a_up[N*FRAMES + UP_OFF];
static int down_rx, up_rx;

struct l2cap_chan *l2cap_connect(u16 p,u16 h,u16 m,const struct l2cap_ops*o,void*x)
{(void)p;(void)m;(void)o;(void)x;return h==1?&ca:&cb;}
int l2cap_send(struct l2cap_chan*c,const void*d,u16 l)
{struct tws*t=(c==&ca)?&b:&a;struct bt_buf*buf=bt_buf_alloc(0);if(!buf)return -1;bt_buf_append(buf,d,l);tws_rx(t,buf);return 0;}
int l2cap_disconnect(struct l2cap_chan*c){(void)c;return 0;}
struct lmp_link *lmp_link_find(u16 h){(void)h;return NULL;}
int lmp_role_switch(struct lmp_link*l){(void)l;return 0;}
void a2dp_set_relay(struct a2dp*x,void(*f)(const u8*,u16,u32,void*),void*g){(void)x;(void)f;(void)g;}

static void on_down(const u8*d,u16 l,u32 ts,void*x){(void)x;memcpy(&b_down[ts+DOWN_OFF],d,l);down_rx++;}
static void on_up(const u8*d,u16 l,u32 ts,void*x){(void)x;memcpy(&a_up[ts+UP_OFF],d,l);up_rx++;}
static void on_key(u8 k,u8 s,void*x){(void)k;(void)s;(void)x;}
static void on_bat(u8 p,void*x){(void)p;(void)x;}

int main(void)
{
    struct tws_callbacks cb={.on_media=on_down,.on_media_up=on_up,.on_key=on_key,.on_battery=on_bat};
    bt_addr pa={{1,2,3,4,5,6}},pb={{6,5,4,3,2,1}},ph={{0x11,0x22,0x33,0x44,0x55,0x66}};
    for(int i=0;i<N*FRAMES;i++){down[i]=(s16)(8000*((i/20)%2?1:-1));up[i]=(s16)(4000*((i/10)%2?1:-1));}
    tws_init(&a,TWS_ROLE_PRIMARY,&cb,NULL);tws_init(&b,TWS_ROLE_SECONDARY,&cb,NULL);
    tws_connect(&a,&pb,1);tws_connect(&b,&pa,2);tws_hello(&a,&ph,0x0F);
    tws_sync(&a,DOWN_OFF,0);tws_sync(&b,UP_OFF,0);
    for(int f=0;f<FRAMES;f++){
        u32 ts=(u32)f*N;
        tws_relay_media(&a,&down[ts],N*2,ts);
        tws_relay_media_up(&b,&up[ts],N*2,ts);
    }
    u32 dm=0,um=0;
    for(int i=0;i<N*FRAMES;i++){if(down[i]!=b_down[i+DOWN_OFF])dm++;if(up[i]!=a_up[i+UP_OFF])um++;}
    printf("downlink frames=%d mism=%u\n",down_rx,dm);
    printf("uplink   frames=%d mism=%u\n",up_rx,um);
    printf("bidir sync: %s\n",(dm==0&&um==0&&down_rx==FRAMES&&up_rx==FRAMES)?"PASS":"FAIL");
    return (dm==0&&um==0&&down_rx==FRAMES&&up_rx==FRAMES)?0:1;
}
