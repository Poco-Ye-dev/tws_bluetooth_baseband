#include <system/config.h>
#include <core/string.h>
#include <core/errno.h>

struct config_entry {
    char key[CONFIG_KEY_MAX];
    char value[CONFIG_VALUE_MAX];
    bool used;
};

static struct config_entry entries[CONFIG_MAX_ENTRIES];
static u8 entry_count;

int config_init(void)
{
    memset(entries, 0, sizeof(entries));
    entry_count = 0;
    return 0;
}

int config_set(const char *key, const char *value)
{
    if (!key || !value)
        return -EINVAL;

    for (u8 i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].key, key) == 0) {
            strncpy(entries[i].value, value, CONFIG_VALUE_MAX - 1);
            return 0;
        }
    }

    if (entry_count >= CONFIG_MAX_ENTRIES)
        return -ENOSPC;

    struct config_entry *entry = &entries[entry_count++];
    entry->used = true;
    strncpy(entry->key, key, CONFIG_KEY_MAX - 1);
    strncpy(entry->value, value, CONFIG_VALUE_MAX - 1);
    return 0;
}

const char *config_get(const char *key, const char *fallback)
{
    if (!key)
        return fallback;
    for (u8 i = 0; i < entry_count; i++) {
        if (entries[i].used && strcmp(entries[i].key, key) == 0)
            return entries[i].value;
    }
    return fallback;
}

int config_load(const char *text)
{
    const char *p = text;

    if (!text)
        return -EINVAL;

    while (*p) {
        while (*p == '\n' || *p == ';' || *p == '\r' || *p == ' ')
            p++;
        if (*p == '#' || *p == '\0')
            break;

        const char *key = p;
        const char *eq = strchr(p, '=');
        if (!eq)
            break;

        const char *end = eq;
        while (end > key && end[-1] == ' ')
            end--;

        const char *value = eq + 1;
        while (*value == ' ')
            value++;

        const char *v_end = value;
        while (*v_end && *v_end != '\n' && *v_end != ';' && *v_end != '\r')
            v_end++;
        while (v_end > value && v_end[-1] == ' ')
            v_end--;

        char key_buf[CONFIG_KEY_MAX];
        char value_buf[CONFIG_VALUE_MAX];
        u32 key_len = (u32)(end - key);
        u32 value_len = (u32)(v_end - value);

        if (key_len >= sizeof(key_buf))
            key_len = sizeof(key_buf) - 1;
        if (value_len >= sizeof(value_buf))
            value_len = sizeof(value_buf) - 1;

        memcpy(key_buf, key, key_len);
        key_buf[key_len] = '\0';
        memcpy(value_buf, value, value_len);
        value_buf[value_len] = '\0';

        config_set(key_buf, value_buf);
        p = v_end;
    }
    return 0;
}

int config_get_int(const char *key, int fallback)
{
    const char *value = config_get(key, NULL);
    s32 parsed;

    if (!value)
        return fallback;
    const char *end = parse_int(value, &parsed);
    return end && *end == '\0' ? (int)parsed : fallback;
}

bool config_get_bool(const char *key, bool fallback)
{
    const char *value = config_get(key, NULL);
    if (!value)
        return fallback;
    return strcasecmp(value, "1") == 0 || strcasecmp(value, "true") == 0 ||
           strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0;
}

u8 config_count(void)
{
    return entry_count;
}
