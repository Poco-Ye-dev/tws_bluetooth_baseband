#ifndef __BT_DUN_H__
#define __BT_DUN_H__

#include <core/types.h>
#include <bt/rfcomm.h>
#include <bt/l2cap.h>

/*
 * Dial-up networking client: AT modem commands over RFCOMM.
 */

struct dun {
    struct l2cap_chan *l2cap;
    struct rfcomm_mux mux;
    struct rfcomm_dlc *dlc;
    bool connected;
};

int dun_init(struct dun *dun);
int dun_connect(struct dun *dun, u16 handle);
int dun_send(struct dun *dun, const char *cmd);
int dun_dial(struct dun *dun, const char *number);
int dun_hangup(struct dun *dun);
void dun_close(struct dun *dun);

#endif /* __BT_DUN_H__ */
