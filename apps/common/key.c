#include <app/key.h>
#include <driver/gpio.h>
#include <core/string.h>

struct key_slot {
    bool used;
    unsigned int gpio;
    bool active_low;
    u16 long_ms;
    u16 held_ms;
    u16 repeat_ms;
    bool down;
    bool long_sent;
    key_cb_t cb;
    void *arg;
};

static struct key_slot keys[KEY_MAX];

int key_register(u8 id, unsigned int gpio, bool active_low, u16 long_ms,
                 key_cb_t cb, void *arg)
{
    if (id >= KEY_MAX)
        return -1;

    struct key_slot *key = &keys[id];
    memset(key, 0, sizeof(*key));
    key->used = true;
    key->gpio = gpio;
    key->active_low = active_low;
    key->long_ms = long_ms ? long_ms : 1000;
    key->cb = cb;
    key->arg = arg;
    return 0;
}

bool key_is_down(u8 id)
{
    return id < KEY_MAX && keys[id].used && keys[id].down;
}

void key_scan(u32 now_ms)
{
    (void)now_ms;

    for (u8 id = 0; id < KEY_MAX; id++) {
        struct key_slot *key = &keys[id];
        int raw;
        bool pressed;

        if (!key->used)
            continue;

        raw = gpio_get_level(key->gpio);
        if (raw < 0)
            continue;

        pressed = key->active_low ? (raw == 0) : (raw != 0);

        if (pressed == key->down) {
            if (pressed) {
                key->held_ms++;
                key->repeat_ms++;
                if (!key->long_sent && key->held_ms >= key->long_ms) {
                    key->long_sent = true;
                    if (key->cb)
                        key->cb(id, KEY_EVENT_LONG, key->arg);
                } else if (key->long_sent &&
                           key->repeat_ms >= KEY_REPEAT_MS) {
                    key->repeat_ms = 0;
                    if (key->cb)
                        key->cb(id, KEY_EVENT_REPEAT, key->arg);
                }
            }
            continue;
        }

        key->down = pressed;
        key->held_ms = 0;
        key->repeat_ms = 0;

        if (pressed) {
            key->long_sent = false;
            if (key->cb)
                key->cb(id, KEY_EVENT_DOWN, key->arg);
        } else {
            if (key->cb)
                key->cb(id, KEY_EVENT_UP, key->arg);
        }
    }
}
