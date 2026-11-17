#include <bt/dun.h>
#include <core/string.h>

static void dun_data(struct rfcomm_dlc *dlc, struct bt_buf *buf)
{
    (void)dlc;
    bt_buf_put(buf);
}

static const struct rfcomm_ops dun_ops = { .data = dun_data };

int dun_init(struct dun *dun)
{
    if (!dun)
        return -1;
    memset(dun, 0, sizeof(*dun));
    return rfcomm_mux_init(&dun->mux);
}

int dun_connect(struct dun *dun, u16 handle)
{
    if (!dun)
        return -1;
    dun->l2cap = l2cap_connect(RFCOMM_PSM, handle, 0, NULL, NULL);
    if (!dun->l2cap)
        return -2;
    rfcomm_mux_attach(&dun->mux, dun->l2cap);
    dun->dlc = rfcomm_connect(&dun->mux, 1, &dun_ops, dun);
    dun->connected = dun->dlc != NULL;
    return dun->dlc ? 0 : -3;
}

int dun_send(struct dun *dun, const char *cmd)
{
    if (!dun || !dun->dlc || !cmd)
        return -1;
    return rfcomm_send(dun->dlc, cmd, (u16)strlen(cmd));
}

int dun_dial(struct dun *dun, const char *number)
{
    char cmd[64];
    u16 pos = 0;

    if (!number)
        return -1;
    cmd[pos++] = 'A'; cmd[pos++] = 'T'; cmd[pos++] = 'D';
    while (*number && pos < sizeof(cmd) - 3)
        cmd[pos++] = *number++;
    cmd[pos++] = '\r';
    cmd[pos] = '\0';
    return dun_send(dun, cmd);
}

int dun_hangup(struct dun *dun)
{
    return dun_send(dun, "ATH\r");
}

void dun_close(struct dun *dun)
{
    if (!dun)
        return;
    if (dun->dlc)
        rfcomm_disconnect(dun->dlc);
    if (dun->l2cap)
        l2cap_disconnect(dun->l2cap);
    dun->connected = false;
}
