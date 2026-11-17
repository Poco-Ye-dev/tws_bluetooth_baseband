#ifndef __CORE_MEM_H__
#define __CORE_MEM_H__

#include <core/types.h>

/*
 * Two allocators share one interface:
 *
 *   kmalloc()  general purpose, may block-free and is the default
 *   kzalloc()  zero filled convenience wrapper
 *   kfree()    accept any pointer returned by the above
 *
 * On this class of part RAM is the scarce resource, so allocations are
 * rounded to the platform's minimum granule and the heap reports watermarks
 * through mem_info().
 */

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

size_t mem_avail(void);
size_t mem_peak(void);

struct mem_info {
    size_t total;
    size_t used;
    size_t peak;
    size_t free_blocks;
    size_t largest_free;
};

void mem_info(struct mem_info *info);

#endif /* __CORE_MEM_H__ */
