#ifndef __BT_OBEX_H__
#define __BT_OBEX_H__

#include <core/types.h>
#include <bt/l2cap.h>
#include <bt/rfcomm.h>

/*
 * Minimal OBEX client used by OPP (object push), PBAP (phonebook) and MAP
 * (messages).  Only connect/put/get/disconnect are needed by these profiles.
 */

#define OBEX_CONNECT     0x80
#define OBEX_DISCONNECT  0x81
#define OBEX_PUT         0x02
#define OBEX_PUT_FINAL   0x82
#define OBEX_GET         0x03
#define OBEX_GET_FINAL   0x83
#define OBEX_ABORT       0xFF

struct obex {
    struct l2cap_chan *l2cap;
    struct rfcomm_mux mux;
    struct rfcomm_dlc *dlc;
    u8 opcode;
    bool connected;
    void (*on_data)(const u8 *data, u16 len, void *arg);
    void *arg;
};

int obex_init(struct obex *obex,
              void (*on_data)(const u8 *, u16, void *), void *arg);
int obex_connect(struct obex *obex, u16 handle, u8 dlci);
int obex_put(struct obex *obex, const void *data, u16 len);
int obex_get(struct obex *obex, u8 type, const void *name, u16 name_len);
int obex_disconnect(struct obex *obex);
void obex_rx(struct obex *obex, const u8 *data, u16 len);
void obex_close(struct obex *obex);

#endif /* __BT_OBEX_H__ */
