#ifndef __BT_TYPES_H__
#define __BT_TYPES_H__

#include <core/types.h>

/*
 * Common value types for the Bluetooth stack.
 *
 * The stack is transport agnostic: nothing below this header knows whether
 * HCI runs over UART, USB or a virtual controller.  Addresses and UUIDs are
 * plain byte containers in little-endian wire order.
 */

typedef struct {
    u8 b[6];
} bt_addr;

typedef struct {
    u8 len;
    u8 b[16];
} bt_uuid;

#define BT_ADDR_ANY ((bt_addr){ .b = { 0, 0, 0, 0, 0, 0 } })
#define BT_UUID16(v) \
    ((bt_uuid){ .len = 2, .b = { (u8)((v) & 0xff), (u8)(((v) >> 8) & 0xff) } })

enum bt_status {
    BT_OK = 0x00,
    BT_ERR_UNKNOWN_CMD = 0x01,
    BT_ERR_CONN_FAILED = 0x02,
    BT_ERR_AUTH_FAILED = 0x05,
    BT_ERR_KEY_MISSING = 0x06,
    BT_ERR_MEMORY = 0x07,
    BT_ERR_TIMEOUT = 0x08,
    BT_ERR_BUSY = 0x0C,
    BT_ERR_REJECTED = 0x0D,
    BT_ERR_UNSUPPORTED = 0x11,
    BT_ERR_INVALID_PARAM = 0x12,
    BT_ERR_DISCONNECTED = 0x13,
    BT_ERR_NOT_FOUND = 0x16,
    BT_ERR_LINK_LOSS = 0x16,
    BT_ERR_NOT_AUTH = 0x05,
};

enum bt_link_type {
    BT_LINK_BR_EDR = 0,
    BT_LINK_LE,
};

static inline bool bt_addr_eq(const bt_addr *a, const bt_addr *b)
{
    for (int i = 0; i < 6; i++) {
        if (a->b[i] != b->b[i])
            return false;
    }
    return true;
}

static inline bool bt_addr_is_any(const bt_addr *a)
{
    return bt_addr_eq(a, &BT_ADDR_ANY);
}

static inline bool bt_uuid_eq(const bt_uuid *a, const bt_uuid *b)
{
    if (a->len != b->len)
        return false;
    for (int i = 0; i < a->len; i++) {
        if (a->b[i] != b->b[i])
            return false;
    }
    return true;
}

#endif /* __BT_TYPES_H__ */
