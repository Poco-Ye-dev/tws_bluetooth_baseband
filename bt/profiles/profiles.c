#include <bt/profile.h>
#include <bt/hci.h>
#include <bt/l2cap.h>
#include <bt/spp.h>
#include <bt/hfp.h>
#include <bt/avdtp.h>
#include <bt/avctp.h>
#include <bt/hid.h>
#include <bt/pan.h>
#include <bt/dun.h>
#include <bt/obex.h>
#include <core/printk.h>

static struct spp spp;
static struct hfp hfp;
static struct a2dp a2dp;
static struct avrcp avrcp;
static struct hid hid;
static struct pan pan;
static struct dun dun;
static struct obex opp;
static struct obex pbap;
static struct obex map;

static int profile_init_spp(void) { spp_init(&spp, NULL, NULL); return 0; }
static int profile_init_hfp(void) { hfp_init(&hfp, NULL, NULL); return 0; }
static int profile_init_a2dp(void) { a2dp_init(&a2dp, NULL, NULL); return 0; }
static int profile_init_avrcp(void) { avrcp_init(&avrcp, NULL, NULL); return 0; }
static int profile_init_hid(void) { hid_init(&hid, NULL, NULL); return 0; }
static int profile_init_pan(void) { pan_init(&pan, NULL, NULL, NULL); return 0; }
static int profile_init_dun(void) { dun_init(&dun); return 0; }
static int profile_init_opp(void) { obex_init(&opp, NULL, NULL); return 0; }
static int profile_init_pbap(void) { obex_init(&pbap, NULL, NULL); return 0; }
static int profile_init_map(void) { obex_init(&map, NULL, NULL); return 0; }

static int profile_connect_spp(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? spp_connect(&spp, peer, conn->handle) : -1;
}
static int profile_connect_hfp(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? hfp_connect(&hfp, conn->handle) : -1;
}
static int profile_connect_a2dp(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? a2dp_connect(&a2dp, conn->handle) : -1;
}
static int profile_connect_avrcp(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? avrcp_connect(&avrcp, conn->handle) : -1;
}

static int profile_connect_hid(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? hid_connect(&hid, conn->handle) : -1;
}
static int profile_connect_pan(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? pan_connect(&pan, conn->handle) : -1;
}
static int profile_connect_dun(const bt_addr *peer)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? dun_connect(&dun, conn->handle) : -1;
}
static int profile_connect_obex(struct obex *obex, const bt_addr *peer, u8 dlci)
{
    struct hci_conn *conn = peer ? hci_conn_find_addr(peer) : NULL;
    return conn ? obex_connect(obex, conn->handle, dlci) : -1;
}
static int profile_connect_opp(const bt_addr *peer) { return profile_connect_obex(&opp, peer, 8); }
static int profile_connect_pbap(const bt_addr *peer) { return profile_connect_obex(&pbap, peer, 9); }
static int profile_connect_map(const bt_addr *peer) { return profile_connect_obex(&map, peer, 10); }

static const struct bt_profile_ops spp_ops = {
    .name = "spp", .psm = 0x0003,
    .init = profile_init_spp, .connect = profile_connect_spp,
};
static const struct bt_profile_ops hfp_ops = {
    .name = "hfp", .psm = 0x0003,
    .init = profile_init_hfp, .connect = profile_connect_hfp,
};
static const struct bt_profile_ops a2dp_ops = {
    .name = "a2dp", .psm = 0x0019,
    .init = profile_init_a2dp, .connect = profile_connect_a2dp,
};
static const struct bt_profile_ops avrcp_ops = {
    .name = "avrcp", .psm = 0x0017,
    .init = profile_init_avrcp, .connect = profile_connect_avrcp,
};

static const struct bt_profile_ops hid_ops = {
    .name = "hid", .psm = 0x0011,
    .init = profile_init_hid, .connect = profile_connect_hid,
};
static const struct bt_profile_ops pan_ops = {
    .name = "pan", .psm = 0x000F,
    .init = profile_init_pan, .connect = profile_connect_pan,
};
static const struct bt_profile_ops dun_ops = {
    .name = "dun", .psm = 0x0003,
    .init = profile_init_dun, .connect = profile_connect_dun,
};
static const struct bt_profile_ops opp_ops = {
    .name = "opp", .psm = 0x0003,
    .init = profile_init_opp, .connect = profile_connect_opp,
};
static const struct bt_profile_ops pbap_ops = {
    .name = "pbap", .psm = 0x0003,
    .init = profile_init_pbap, .connect = profile_connect_pbap,
};
static const struct bt_profile_ops map_ops = {
    .name = "map", .psm = 0x0003,
    .init = profile_init_map, .connect = profile_connect_map,
};

void bt_profiles_builtin_register(void)
{
    bt_profile_register(&spp_ops);
    bt_profile_register(&hfp_ops);
    bt_profile_register(&a2dp_ops);
    bt_profile_register(&avrcp_ops);
    bt_profile_register(&hid_ops);
    bt_profile_register(&pan_ops);
    bt_profile_register(&dun_ops);
    bt_profile_register(&opp_ops);
    bt_profile_register(&pbap_ops);
    bt_profile_register(&map_ops);
}
