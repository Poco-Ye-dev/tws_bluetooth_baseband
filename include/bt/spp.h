#ifndef __BT_SPP_H__
#define __BT_SPP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/l2cap.h>
#include <bt/rfcomm.h>

/*
 * Serial Port Profile over RFCOMM.
 */

typedef void (*spp_rx_cb_t)(const u8 *data, u16 len, void *arg);

struct spp {
    bool connected;
    struct l2cap_chan *l2cap;
    struct rfcomm_mux mux;
    struct rfcomm_dlc *dlc;
    spp_rx_cb_t rx;
    void *arg;
};

int spp_init(struct spp *spp, spp_rx_cb_t rx, void *arg);
int spp_connect(struct spp *spp, const bt_addr *peer, u16 handle);
int spp_send(struct spp *spp, const void *data, u16 len);
void spp_rx(struct spp *spp, const u8 *data, u16 len);
void spp_close(struct spp *spp);

#endif /* __BT_SPP_H__ */
