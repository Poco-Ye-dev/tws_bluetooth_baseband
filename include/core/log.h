#ifndef __CORE_LOG_H__
#define __CORE_LOG_H__

#include <core/types.h>

/*
 * In-memory log ring.  printk() mirrors into this buffer so a crash dump or a
 * support snapshot can include the last few hundred lines without a console.
 */

#define LOG_RING_SIZE 4096

struct log_entry {
    u32 timestamp;
    u8 level;
    char text[96];
};

void log_init(void);
void log_write(u8 level, const char *text);
u32 log_count(void);
const struct log_entry *log_get(u32 index);
void log_dump(void);

#endif /* __CORE_LOG_H__ */
