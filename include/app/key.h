#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include <core/types.h>

/*
 * Key scanning lives above GPIO: it owns debounce, long-press and repeat
 * timing so product code only sees semantic events.
 */

#define KEY_MAX 8
#define KEY_REPEAT_MS 200

enum key_event {
    KEY_EVENT_DOWN = 0,
    KEY_EVENT_UP,
    KEY_EVENT_LONG,
    KEY_EVENT_REPEAT,
};

typedef void (*key_cb_t)(u8 id, enum key_event event, void *arg);

int key_register(u8 id, unsigned int gpio, bool active_low, u16 long_ms,
                 key_cb_t cb, void *arg);
void key_scan(u32 now_ms);
bool key_is_down(u8 id);

#endif /* __APP_KEY_H__ */
