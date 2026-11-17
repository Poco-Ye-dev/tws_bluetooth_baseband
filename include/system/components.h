#ifndef __SYSTEM_COMPONENTS_H__
#define __SYSTEM_COMPONENTS_H__

#include <core/types.h>

/*
 * Component registry.
 *
 * Board files register the devices they populate; the kernel brings them up
 * in registration order and tears them down in reverse.  This keeps board
 * specific code out of the generic init path.
 */

#define COMPONENT_NAME_MAX 20
#define COMPONENT_MAX 16

struct component {
    char name[COMPONENT_NAME_MAX];
    int (*init)(void);
    void (*deinit)(void);
    void *priv;
    bool used;
};

int component_register(const char *name, int (*init)(void),
                       void (*deinit)(void), void *priv);
struct component *component_find(const char *name);
int component_init_all(void);
void component_deinit_all(void);
u8 component_count(void);

#endif /* __SYSTEM_COMPONENTS_H__ */
