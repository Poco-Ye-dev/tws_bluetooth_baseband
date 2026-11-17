#ifndef __SYSTEM_CLOCK_H__
#define __SYSTEM_CLOCK_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Clock tree control.  Consumers ask for a rate, the port rounds down to the
 * nearest supported value and returns the achieved rate, which callers must
 * use to compute audio dividers.
 */

enum clock_id {
    CLOCK_CPU = 0,
    CLOCK_BUS,
    CLOCK_AUDIO,
    CLOCK_FLASH,
    CLOCK_COUNT,
};

int clock_init(void);
u32 clock_get(enum clock_id id);
u32 clock_set(enum clock_id id, u32 hz);
int clock_enable(enum clock_id id, bool enable);
bool clock_enabled(enum clock_id id);

#endif /* __SYSTEM_CLOCK_H__ */
