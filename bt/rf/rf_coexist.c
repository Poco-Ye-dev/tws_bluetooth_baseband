#include <bt/rf_coexist.h>

static enum rf_owner current;

void rf_coexist_init(void)
{
    current = RF_OWNER_NONE;
}

bool rf_acquire(enum rf_owner owner)
{
    if (current != RF_OWNER_NONE && current != owner)
        return false;
    current = owner;
    return true;
}

void rf_release(enum rf_owner owner)
{
    if (current == owner)
        current = RF_OWNER_NONE;
}

enum rf_owner rf_owner(void)
{
    return current;
}
