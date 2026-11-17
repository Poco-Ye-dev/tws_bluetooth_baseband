#include <bt/gatt.h>
#include <core/string.h>
#include <core/printk.h>

#define GATT_VALUE_SLOTS 32

struct value_slot {
    u16 handle;
    u16 len;
    u8 data[32];
    bool used;
};

static struct value_slot values[GATT_VALUE_SLOTS];

static struct value_slot *slot_for(u16 handle, bool create)
{
    for (unsigned int i = 0; i < GATT_VALUE_SLOTS; i++) {
        if (values[i].used && values[i].handle == handle)
            return &values[i];
    }
    if (!create)
        return NULL;

    for (unsigned int i = 0; i < GATT_VALUE_SLOTS; i++) {
        if (!values[i].used) {
            memset(&values[i], 0, sizeof(values[i]));
            values[i].used = true;
            values[i].handle = handle;
            return &values[i];
        }
    }
    return NULL;
}

static int service_read(u16 handle, void *buf, u16 max, void *arg)
{
    struct gatt_service_def *svc = arg;

    (void)handle;
    if (svc->uuid.len > max)
        return -1;
    memcpy(buf, svc->uuid.b, svc->uuid.len);
    return svc->uuid.len;
}

static int char_decl_read(u16 handle, void *buf, u16 max, void *arg)
{
    struct gatt_char_def *ch = arg;

    (void)handle;
    if (max < 5 + ch->uuid.len)
        return -1;

    u8 *out = buf;
    out[0] = ch->props;
    out[1] = (u8)(ch->value_handle & 0xFF);
    out[2] = (u8)(ch->value_handle >> 8);
    memcpy(&out[3], ch->uuid.b, ch->uuid.len);
    return 5 + ch->uuid.len;
}

static int value_read(u16 handle, void *buf, u16 max, void *arg)
{
    struct value_slot *slot = slot_for(handle, false);

    (void)arg;
    if (!slot)
        return 0;
    if (slot->len > max)
        return -1;
    memcpy(buf, slot->data, slot->len);
    return slot->len;
}

static int value_write(u16 handle, const void *buf, u16 len, void *arg)
{
    struct value_slot *slot = slot_for(handle, true);

    (void)arg;
    if (!slot || len > sizeof(slot->data))
        return -1;
    memcpy(slot->data, buf, len);
    slot->len = len;
    return 0;
}

int gatt_server_init(struct gatt_server *server, struct att_db *db)
{
    if (!server || !db)
        return -1;

    memset(server, 0, sizeof(*server));
    server->db = db;
    memset(values, 0, sizeof(values));
    return 0;
}

int gatt_add_service(struct gatt_server *server, struct gatt_service_def *svc)
{
    if (!server || !svc || server->service_count >= GATT_MAX_SERVICES)
        return -1;

    bt_uuid primary = BT_UUID16(GATT_UUID_PRIMARY_SERVICE);
    svc->handle = att_add_attr(server->db, &primary, ATT_PROP_READ,
                               service_read, NULL, svc);

    for (unsigned int i = 0; i < svc->char_count; i++) {
        struct gatt_char_def *ch = &svc->chars[i];
        bt_uuid decl = BT_UUID16(GATT_UUID_CHARACTERISTIC);

        u16 decl_handle = att_add_attr(server->db, &decl, ATT_PROP_READ,
                                       char_decl_read, NULL, ch);
        if (!decl_handle)
            return -2;

        ch->value_handle = att_add_attr(server->db, &ch->uuid, ch->props,
                                        value_read, value_write, NULL);
        if (!ch->value_handle)
            return -3;

        slot_for(ch->value_handle, true);

        if (ch->props & (ATT_PROP_NOTIFY | ATT_PROP_INDICATE)) {
            bt_uuid cccd = BT_UUID16(GATT_UUID_CCCD);
            att_add_attr(server->db, &cccd, ATT_PROP_READ | ATT_PROP_WRITE,
                         value_read, value_write, NULL);
        }
    }

    server->service[server->service_count++] = svc;
    pr_info("gatt: service 0x%04x registered\n",
            svc->uuid.len == 2 ? (svc->uuid.b[0] | (svc->uuid.b[1] << 8)) : 0);
    return 0;
}

struct gatt_service_def *gatt_find_service(struct gatt_server *server,
                                           const bt_uuid *uuid)
{
    for (unsigned int i = 0; i < server->service_count; i++) {
        if (bt_uuid_eq(&server->service[i]->uuid, uuid))
            return server->service[i];
    }
    return NULL;
}

int gatt_set_value(struct gatt_service_def *svc, u16 value_handle,
                   const void *data, u16 len)
{
    (void)svc;

    struct value_slot *slot = slot_for(value_handle, true);
    if (!slot || len > sizeof(slot->data))
        return -1;
    memcpy(slot->data, data, len);
    slot->len = len;
    return 0;
}

int gatt_notify(struct gatt_server *server, struct gatt_service_def *svc,
                u16 value_handle, const void *data, u16 len)
{
    if (!server || !svc)
        return -1;

    gatt_set_value(svc, value_handle, data, len);
    return att_notify(server->db, value_handle, data, len);
}

int gatt_discover_services(struct l2cap_chan *chan,
                           struct gatt_discovery *discovery)
{
    bt_uuid primary = BT_UUID16(GATT_UUID_PRIMARY_SERVICE);

    if (!chan || !discovery)
        return -1;

    discovery->uuid = primary;
    discovery->start_handle = 1;
    discovery->end_handle = 0xFFFF;
    return att_client_read_by_group(chan, discovery->start_handle,
                                    discovery->end_handle, &primary);
}
