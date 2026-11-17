#ifndef __BT_HFP_H__
#define __BT_HFP_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/rfcomm.h>
#include <bt/l2cap.h>

/*
 * Hands-Free Profile AG side: SLC establishment over RFCOMM and the AT
 * command subset needed for call control and volume.
 */

#define HFP_AT_MAX 96

enum hfp_state {
    HFP_IDLE = 0,
    HFP_WAIT_SLC,
    HFP_READY,
    HFP_CALL_ACTIVE,
};

struct hfp {
    enum hfp_state state;
    struct l2cap_chan *l2cap;
    struct rfcomm_mux mux;
    struct rfcomm_dlc *dlc;
    char line[HFP_AT_MAX];
    u16 line_len;
    u8 volume_speaker;
    u8 volume_mic;
    u16 sco_handle;
    void (*on_call)(bool active, void *arg);
    void *arg;
};

int hfp_init(struct hfp *hfp, void (*on_call)(bool, void *), void *arg);
int hfp_connect(struct hfp *hfp, u16 handle);
int hfp_send(struct hfp *hfp, const char *at);
int hfp_answer(struct hfp *hfp);
int hfp_hangup(struct hfp *hfp);
int hfp_dial(struct hfp *hfp, const char *number);
int hfp_set_volume(struct hfp *hfp, u8 percent);
void hfp_rx(struct hfp *hfp, const u8 *data, u16 len);
void hfp_close(struct hfp *hfp);

#endif /* __BT_HFP_H__ */
