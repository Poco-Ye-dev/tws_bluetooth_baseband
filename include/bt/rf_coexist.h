#ifndef __BT_RF_COEXIST_H__
#define __BT_RF_COEXIST_H__

#include <core/types.h>

/*
 * Dual mode RF arbitration.
 *
 * BR/EDR and LE are separate baseband engines but share the antenna and the
 * analog front end.  A tick must own the RF for its slot; the other engine
 * waits for the next opportunity.
 */

enum rf_owner {
    RF_OWNER_NONE = 0,
    RF_OWNER_EDR,
    RF_OWNER_LE,
};

void rf_coexist_init(void);
bool rf_acquire(enum rf_owner owner);
void rf_release(enum rf_owner owner);
enum rf_owner rf_owner(void);

#endif /* __BT_RF_COEXIST_H__ */
