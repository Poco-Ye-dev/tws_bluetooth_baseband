#ifndef __BT_GATT_H__
#define __BT_GATT_H__

#include <core/types.h>
#include <bt/types.h>
#include <bt/att.h>

/*
 * GATT service model on top of the ATT attribute table.
 *
 * A service is declared with an array of characteristics; the core assigns
 * handles, creates the declaration/value/CCCD attributes and keeps the
 * value callbacks private to the service.
 */

#define GATT_MAX_SERVICES 8
#define GATT_MAX_CHARS    16

#define GATT_UUID_PRIMARY_SERVICE 0x2800
#define GATT_UUID_CHARACTERISTIC  0x2803
#define GATT_UUID_CCCD            0x2902
#define GATT_UUID_DEVICE_NAME     0x2A00

struct gatt_char_def {
    bt_uuid uuid;
    u8 props;
    const char *name;
    u16 value_handle;
    void *arg;
};

struct gatt_service_def {
    bt_uuid uuid;
    struct gatt_char_def chars[GATT_MAX_CHARS];
    u8 char_count;
    u16 handle;
};

struct gatt_server {
    struct att_db *db;
    struct gatt_service_def *service[GATT_MAX_SERVICES];
    u8 service_count;
};

int gatt_server_init(struct gatt_server *server, struct att_db *db);
int gatt_add_service(struct gatt_server *server, struct gatt_service_def *svc);
struct gatt_service_def *gatt_find_service(struct gatt_server *server,
                                           const bt_uuid *uuid);
int gatt_set_value(struct gatt_service_def *svc, u16 value_handle,
                   const void *data, u16 len);
int gatt_notify(struct gatt_server *server, struct gatt_service_def *svc,
                u16 value_handle, const void *data, u16 len);

/* Client side discovery results. */
struct gatt_discovery {
    u16 start_handle;
    u16 end_handle;
    bt_uuid uuid;
    void (*on_service)(const struct gatt_discovery *svc, void *arg);
    void *arg;
};

int gatt_discover_services(struct l2cap_chan *chan,
                           struct gatt_discovery *discovery);

#endif /* __BT_GATT_H__ */
