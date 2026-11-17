#include <bt/hfp.h>
#include <bt/baseband.h>
#include <core/string.h>

static void hfp_data(struct rfcomm_dlc *dlc, struct bt_buf *buf)
{
    struct hfp *hfp = dlc ? dlc->arg : NULL;
    if (hfp)
        hfp_rx(hfp, buf->data, bt_buf_len(buf));
    bt_buf_put(buf);
}

static const struct rfcomm_ops hfp_ops = { .data = hfp_data };

int hfp_init(struct hfp *hfp, void (*on_call)(bool, void *), void *arg)
{
    if (!hfp)
        return -1;
    memset(hfp, 0, sizeof(*hfp));
    hfp->state = HFP_IDLE;
    hfp->on_call = on_call;
    hfp->arg = arg;
    return rfcomm_mux_init(&hfp->mux);
}

int hfp_connect(struct hfp *hfp, u16 handle)
{
    if (!hfp)
        return -1;
    hfp->l2cap = l2cap_connect(RFCOMM_PSM, handle, 0, NULL, NULL);
    if (!hfp->l2cap)
        return -2;
    rfcomm_mux_attach(&hfp->mux, hfp->l2cap);
    hfp->dlc = rfcomm_connect(&hfp->mux, 3, &hfp_ops, hfp);
    hfp->state = HFP_WAIT_SLC;
    return hfp->dlc ? 0 : -3;
}

int hfp_send(struct hfp *hfp, const char *at)
{
    if (!hfp || !at || !hfp->dlc)
        return -1;
    return rfcomm_send(hfp->dlc, at, (u16)strlen(at));
}

static void line_complete(struct hfp *hfp)
{
    hfp->line[hfp->line_len] = '\0';

    if (strncmp(hfp->line, "AT+BRSF", 7) == 0) {
        hfp_send(hfp, "\r\n+BRSF: 0x0CE\r\n");
        hfp_send(hfp, "\r\nOK\r\n");
    } else if (strncmp(hfp->line, "AT+CIND=?", 9) == 0) {
        hfp_send(hfp, "\r\n+CIND: (\"call\",(0,1)),(\"callsetup\",(0,3)),(\"battchg\",(0,5))\r\n");
        hfp_send(hfp, "\r\nOK\r\n");
    } else if (strncmp(hfp->line, "AT+CIND?", 8) == 0) {
        hfp_send(hfp, "\r\n+CIND: 0,0,5\r\n");
        hfp_send(hfp, "\r\nOK\r\n");
    } else if (strncmp(hfp->line, "AT+CMER", 7) == 0) {
        hfp_send(hfp, "\r\nOK\r\n");
        hfp->state = HFP_READY;
    } else if (strncmp(hfp->line, "AT+VGS=", 7) == 0) {
        hfp->volume_speaker = (u8)(hfp->line[7] - '0');
        hfp_send(hfp, "\r\nOK\r\n");
    } else if (strncmp(hfp->line, "ATA", 3) == 0) {
        hfp->state = HFP_CALL_ACTIVE;
        hfp_send(hfp, "\r\nOK\r\n");
        if (hfp->on_call)
            hfp->on_call(true, hfp->arg);
    } else if (strncmp(hfp->line, "AT+CHUP", 7) == 0 ||
               strncmp(hfp->line, "AT+CHLD=1", 9) == 0) {
        hfp->state = HFP_READY;
        hfp_send(hfp, "\r\nOK\r\n");
        if (hfp->on_call)
            hfp->on_call(false, hfp->arg);
    } else if (strncmp(hfp->line, "ATD", 3) == 0) {
        hfp_send(hfp, "\r\nOK\r\n");
        if (hfp->on_call)
            hfp->on_call(true, hfp->arg);
    } else {
        hfp_send(hfp, "\r\nOK\r\n");
    }

    hfp->line_len = 0;
}

void hfp_rx(struct hfp *hfp, const u8 *data, u16 len)
{
    if (!hfp || !data)
        return;

    for (u16 i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c == '\r' || c == '\n') {
            if (hfp->line_len)
                line_complete(hfp);
            continue;
        }
        if (hfp->line_len < HFP_AT_MAX - 1)
            hfp->line[hfp->line_len++] = c;
    }
}

int hfp_answer(struct hfp *hfp)
{
    return hfp_send(hfp, "ATA\r");
}

int hfp_hangup(struct hfp *hfp)
{
    return hfp_send(hfp, "AT+CHUP\r");
}

int hfp_dial(struct hfp *hfp, const char *number)
{
    char cmd[HFP_AT_MAX];
    u16 pos = 0;

    if (!number)
        return -1;
    cmd[pos++] = 'A';
    cmd[pos++] = 'T';
    cmd[pos++] = 'D';
    while (*number && pos < HFP_AT_MAX - 3)
        cmd[pos++] = *number++;
    cmd[pos++] = ';';
    cmd[pos++] = '\r';
    cmd[pos] = '\0';
    return hfp_send(hfp, cmd);
}

int hfp_set_volume(struct hfp *hfp, u8 percent)
{
    char cmd[16];
    if (!hfp || percent > 100)
        return -1;
    hfp->volume_speaker = percent;
    u16 pos = 0;
    cmd[pos++] = 'A'; cmd[pos++] = 'T'; cmd[pos++] = '+';
    cmd[pos++] = 'V'; cmd[pos++] = 'G'; cmd[pos++] = 'S'; cmd[pos++] = '=';
    if (percent >= 100)
        cmd[pos++] = '1';
    if (percent >= 10)
        cmd[pos++] = (char)('0' + (percent / 10) % 10);
    cmd[pos++] = (char)('0' + percent % 10);
    cmd[pos++] = '\r';
    cmd[pos] = '\0';
    return hfp_send(hfp, cmd);
}

void hfp_close(struct hfp *hfp)
{
    if (!hfp)
        return;
    if (hfp->dlc)
        rfcomm_disconnect(hfp->dlc);
    if (hfp->l2cap)
        l2cap_disconnect(hfp->l2cap);
    hfp->state = HFP_IDLE;
}
