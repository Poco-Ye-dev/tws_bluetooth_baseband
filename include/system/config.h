#ifndef __SYSTEM_CONFIG_H__
#define __SYSTEM_CONFIG_H__

#include <core/types.h>

/*
 * Key/value configuration with typed accessors.
 *
 * The parser accepts "key=value" lines separated by newlines or semicolons,
 * ignores comments starting with '#', and stores values as strings so the
 * same table serves flash configuration and factory test overrides.
 */

#define CONFIG_KEY_MAX 24
#define CONFIG_VALUE_MAX 40
#define CONFIG_MAX_ENTRIES 24

int config_init(void);
int config_load(const char *text);
int config_set(const char *key, const char *value);
const char *config_get(const char *key, const char *fallback);
int config_get_int(const char *key, int fallback);
bool config_get_bool(const char *key, bool fallback);
u8 config_count(void);

#endif /* __SYSTEM_CONFIG_H__ */
