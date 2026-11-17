#ifndef __BT_PROFILE_H__
#define __BT_PROFILE_H__

#include <core/types.h>
#include <bt/types.h>

/*
 * Profile contract.  A profile owns one or more PSMs and reacts to connection
 * events; it never touches HCI directly.  Registration is data driven so a
 * product build simply omits the profiles it does not ship.
 */

struct bt_profile_ops {
    const char *name;
    u16 psm;
    int (*init)(void);
    int (*connect)(const bt_addr *peer);
    int (*disconnect)(void);
};

int bt_profile_register(const struct bt_profile_ops *ops);
int bt_profile_init_all(void);
const struct bt_profile_ops *bt_profile_find(const char *name);
u8 bt_profile_count(void);

/* Registers the profiles compiled into this build. */
void bt_profiles_builtin_register(void);

#endif /* __BT_PROFILE_H__ */
