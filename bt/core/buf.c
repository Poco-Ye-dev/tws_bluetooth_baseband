#include <bt/buf.h>
#include <core/string.h>

struct bt_buf_slot {
    struct bt_buf buf;
    u8 storage[BT_BUF_SIZE];
};

static struct bt_buf_slot pool[BT_BUF_POOL];
static bool initialized;

void bt_buf_init(void)
{
    for (unsigned int i = 0; i < BT_BUF_POOL; i++) {
        memset(&pool[i].buf, 0, sizeof(pool[i].buf));
        pool[i].buf.pool_index = (u8)i;
        pool[i].buf.refs = 0;
    }
    initialized = true;
}

struct bt_buf *bt_buf_alloc(u16 headroom)
{
    struct bt_buf *buf;

    if (!initialized)
        bt_buf_init();

    if (headroom > BT_BUF_HEADROOM)
        return NULL;

    for (unsigned int i = 0; i < BT_BUF_POOL; i++) {
        if (pool[i].buf.refs)
            continue;

        buf = &pool[i].buf;
        buf->data = pool[i].storage + headroom;
        buf->tail = buf->data;
        buf->end = pool[i].storage + BT_BUF_SIZE;
        buf->next = NULL;
        buf->refs = 1;
        buf->flags = 0;
        return buf;
    }
    return NULL;
}

struct bt_buf *bt_buf_clone(struct bt_buf *buf)
{
    struct bt_buf *copy;
    u16 headroom;
    u16 len;

    if (!buf)
        return NULL;

    headroom = bt_buf_headroom(buf);
    copy = bt_buf_alloc(headroom);
    if (!copy)
        return NULL;

    len = bt_buf_len(buf);
    if (bt_buf_append(copy, buf->data, len) != len) {
        bt_buf_put(copy);
        return NULL;
    }
    copy->flags = buf->flags;
    return copy;
}

void bt_buf_get(struct bt_buf *buf)
{
    if (buf)
        buf->refs++;
}

void bt_buf_put(struct bt_buf *buf)
{
    if (!buf || buf->refs == 0)
        return;
    if (--buf->refs)
        return;

    buf->data = NULL;
    buf->tail = NULL;
    buf->end = NULL;
    buf->next = NULL;
}

u16 bt_buf_len(const struct bt_buf *buf)
{
    return (u16)(buf->tail - buf->data);
}

u16 bt_buf_headroom(const struct bt_buf *buf)
{
    struct bt_buf_slot *slot = &pool[buf->pool_index];
    return (u16)(buf->data - slot->storage);
}

void *bt_buf_push(struct bt_buf *buf, u16 len)
{
    struct bt_buf_slot *slot = &pool[buf->pool_index];

    if (buf->data - len < slot->storage)
        return NULL;

    buf->data -= len;
    return buf->data;
}

void *bt_buf_put_data(struct bt_buf *buf, u16 len)
{
    void *at;

    if (buf->tail + len > buf->end)
        return NULL;

    at = buf->tail;
    buf->tail += len;
    return at;
}

void *bt_buf_pull(struct bt_buf *buf, u16 len)
{
    void *at;

    if (buf->data + len > buf->tail)
        return NULL;

    at = buf->data;
    buf->data += len;
    return at;
}

void bt_buf_trim(struct bt_buf *buf, u16 len)
{
    if (len > bt_buf_len(buf))
        len = bt_buf_len(buf);
    buf->tail -= len;
}

u16 bt_buf_append(struct bt_buf *buf, const void *data, u16 len)
{
    void *dst = bt_buf_put_data(buf, len);

    if (!dst)
        return 0;
    if (data && len)
        memcpy(dst, data, len);
    return len;
}

u16 bt_buf_read(struct bt_buf *buf, void *dst, u16 len)
{
    void *src = bt_buf_pull(buf, len);

    if (!src)
        return 0;
    if (dst && len)
        memcpy(dst, src, len);
    return len;
}

u8 bt_buf_get_u8(struct bt_buf *buf)
{
    u8 value = 0;
    bt_buf_read(buf, &value, 1);
    return value;
}

u16 bt_buf_get_le16(struct bt_buf *buf)
{
    u8 raw[2] = { 0, 0 };

    bt_buf_read(buf, raw, 2);
    return (u16)(raw[0] | (raw[1] << 8));
}

void bt_buf_list_append(struct bt_buf **head, struct bt_buf *buf)
{
    struct bt_buf **at = head;

    buf->next = NULL;
    while (*at)
        at = &(*at)->next;
    *at = buf;
}

struct bt_buf *bt_buf_list_pop(struct bt_buf **head)
{
    struct bt_buf *buf = *head;

    if (buf)
        *head = buf->next;
    return buf;
}

u16 bt_buf_list_len(struct bt_buf *head)
{
    u16 total = 0;
    while (head) {
        total = (u16)(total + bt_buf_len(head));
        head = head->next;
    }
    return total;
}
