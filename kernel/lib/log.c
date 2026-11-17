#include <core/log.h>
#include <core/string.h>
#include <core/printk.h>

/*
 * Fixed-size log ring.  Entries are overwritten oldest-first; a sequence
 * counter lets a reader detect wrap-around and dump in chronological order.
 */

static struct log_entry entries[LOG_RING_SIZE / sizeof(struct log_entry)];
static u32 head;
static u32 count;
static u32 sequence;
static bool ready;

void log_init(void)
{
    memset(entries, 0, sizeof(entries));
    head = 0;
    count = 0;
    sequence = 0;
    ready = true;
}

void log_write(u8 level, const char *text)
{
    struct log_entry *entry;

    if (!ready)
        log_init();
    if (!text)
        return;

    entry = &entries[head];
    entry->timestamp = sequence++;
    entry->level = level;
    strncpy(entry->text, text, sizeof(entry->text) - 1);
    entry->text[sizeof(entry->text) - 1] = '\0';

    head = (head + 1) % (sizeof(entries) / sizeof(entries[0]));
    if (count < sizeof(entries) / sizeof(entries[0]))
        count++;
}

u32 log_count(void)
{
    return count;
}

const struct log_entry *log_get(u32 index)
{
    u32 slots = sizeof(entries) / sizeof(entries[0]);

    if (index >= count)
        return NULL;
    if (count < slots)
        return &entries[index];
    return &entries[(head + index) % slots];
}

void log_dump(void)
{
    for (u32 i = 0; i < log_count(); i++) {
        const struct log_entry *entry = log_get(i);
        if (entry)
            printk("[%u] %s\n", entry->timestamp, entry->text);
    }
}
