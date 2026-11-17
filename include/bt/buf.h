#ifndef __BT_BUF_H__
#define __BT_BUF_H__

#include <core/types.h>

/*
 * Reference counted packet buffers with headroom.
 *
 * Every layer prepends its own header by moving `data` backwards, so a frame
 * travels from ATT down to HCI without a single copy.  Buffers are carved
 * from a fixed pool; running out is a normal, recoverable condition.
 */

#define BT_BUF_SIZE      512u
#define BT_BUF_HEADROOM  64u
#define BT_BUF_POOL      48u

struct bt_buf {
    struct bt_buf *next;
    u8 *data;
    u8 *tail;
    u8 *end;
    u16 refs;
    u8 flags;
    u8 pool_index;
};

/* Flags carried between layers. */
#define BT_BUF_F_ACL      0x01
#define BT_BUF_F_LE       0x02
#define BT_BUF_F_START    0x04
#define BT_BUF_F_COMPLETE 0x08

void bt_buf_init(void);
struct bt_buf *bt_buf_alloc(u16 headroom);
struct bt_buf *bt_buf_clone(struct bt_buf *buf);
void bt_buf_get(struct bt_buf *buf);
void bt_buf_put(struct bt_buf *buf);
u16 bt_buf_len(const struct bt_buf *buf);
u16 bt_buf_headroom(const struct bt_buf *buf);

void *bt_buf_push(struct bt_buf *buf, u16 len);
void *bt_buf_put_data(struct bt_buf *buf, u16 len);
void *bt_buf_pull(struct bt_buf *buf, u16 len);
void bt_buf_trim(struct bt_buf *buf, u16 len);

u16 bt_buf_append(struct bt_buf *buf, const void *data, u16 len);
u16 bt_buf_read(struct bt_buf *buf, void *dst, u16 len);
u8 bt_buf_get_u8(struct bt_buf *buf);
u16 bt_buf_get_le16(struct bt_buf *buf);

/* Intrusive singly linked list helpers for fragment queues. */
void bt_buf_list_append(struct bt_buf **head, struct bt_buf *buf);
struct bt_buf *bt_buf_list_pop(struct bt_buf **head);
u16 bt_buf_list_len(struct bt_buf *head);

#endif /* __BT_BUF_H__ */
