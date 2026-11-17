#include <bt/stack.h>
#include <bt/hci.h>
#include <bt/l2cap.h>
#include <bt/gap.h>
#include <bt/profile.h>
#include <core/string.h>

#define BT_MAX_SUBSCRIBERS 8
#define BT_MAX_PROFILES    8

struct subscriber {
    bt_event_cb_t cb;
    void *arg;
};

static struct {
    const struct bt_transport *transport;
    bool started;
    struct subscriber subs[BT_MAX_SUBSCRIBERS];
    u8 sub_count;
    const struct bt_profile_ops *profiles[BT_MAX_PROFILES];
    u8 profile_count;
} stack;

int bt_stack_init(const struct bt_transport *transport)
{
    if (!transport)
        return -1;

    memset(&stack, 0, sizeof(stack));
    stack.transport = transport;

    hci_init();
    if (bt_transport_register(transport))
        return -2;

    l2cap_init();
    gap_init(NULL);
    gap_register_handlers();
    bt_profiles_builtin_register();
    return 0;
}

int bt_stack_start(void)
{
    int ret;

    if (!stack.transport)
        return -1;

    ret = hci_open();
    if (ret)
        return ret;

    bt_profile_init_all();
    stack.started = true;
    return 0;
}

void bt_stack_stop(void)
{
    if (!stack.started)
        return;
    hci_close();
    stack.started = false;
}

void bt_stack_poll(void)
{
    hci_poll();
}

bool bt_stack_ready(void)
{
    return hci_ready();
}

const bt_addr *bt_stack_local_addr(void)
{
    return hci_local_addr();
}

int bt_subscribe(bt_event_cb_t cb, void *arg)
{
    if (!cb || stack.sub_count >= BT_MAX_SUBSCRIBERS)
        return -1;

    stack.subs[stack.sub_count].cb = cb;
    stack.subs[stack.sub_count].arg = arg;
    stack.sub_count++;
    return 0;
}

void bt_publish(enum bt_event event, const void *data)
{
    for (unsigned int i = 0; i < stack.sub_count; i++) {
        if (stack.subs[i].cb)
            stack.subs[i].cb(event, data, stack.subs[i].arg);
    }
}

int bt_profile_register(const struct bt_profile_ops *ops)
{
    if (!ops || stack.profile_count >= BT_MAX_PROFILES)
        return -1;

    stack.profiles[stack.profile_count++] = ops;
    return 0;
}

int bt_profile_init_all(void)
{
    int result = 0;

    for (unsigned int i = 0; i < stack.profile_count; i++) {
        const struct bt_profile_ops *ops = stack.profiles[i];
        if (ops->init) {
            int ret = ops->init();
            if (ret)
                result = ret;
        }
    }
    return result;
}

const struct bt_profile_ops *bt_profile_find(const char *name)
{
    for (unsigned int i = 0; i < stack.profile_count; i++) {
        if (strcmp(stack.profiles[i]->name, name) == 0)
            return stack.profiles[i];
    }
    return NULL;
}

u8 bt_profile_count(void)
{
    return stack.profile_count;
}
