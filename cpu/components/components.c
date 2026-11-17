#include <system/components.h>
#include <core/string.h>
#include <core/printk.h>
#include <core/errno.h>

static struct component components[COMPONENT_MAX];
static u8 count;

int component_register(const char *name, int (*init)(void),
                       void (*deinit)(void), void *priv)
{
    if (!name || count >= COMPONENT_MAX)
        return -ENOSPC;

    struct component *c = &components[count];
    strncpy(c->name, name, COMPONENT_NAME_MAX - 1);
    c->init = init;
    c->deinit = deinit;
    c->priv = priv;
    c->used = true;
    count++;
    return 0;
}

struct component *component_find(const char *name)
{
    for (u8 i = 0; i < count; i++) {
        if (components[i].used && strcmp(components[i].name, name) == 0)
            return &components[i];
    }
    return NULL;
}

int component_init_all(void)
{
    int result = 0;

    for (u8 i = 0; i < count; i++) {
        struct component *c = &components[i];
        if (!c->used || !c->init)
            continue;
        int ret = c->init();
        if (ret) {
            pr_err("component %s init failed: %d\n", c->name, ret);
            result = ret;
        } else {
            pr_info("component %s ready\n", c->name);
        }
    }
    return result;
}

void component_deinit_all(void)
{
    for (int i = (int)count - 1; i >= 0; i--) {
        struct component *c = &components[i];
        if (c->used && c->deinit)
            c->deinit();
    }
}

u8 component_count(void)
{
    return count;
}
