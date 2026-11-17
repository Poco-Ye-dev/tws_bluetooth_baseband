#ifndef __UTILS_FS_H__
#define __UTILS_FS_H__

#include <core/types.h>

/*
 * Tiny read-mostly filesystem.
 *
 * A backend describes a byte range on flash (or RAM); the VFS maps paths to
 * ranges and tracks open descriptors.  Writes go through the backend so the
 * same API serves configuration, prompts and OTA staging.
 */

#define FS_PATH_MAX 32
#define FS_MAX_FILES 16
#define FS_MAX_BACKENDS 4

#define FS_O_RDONLY 0x01
#define FS_O_WRONLY 0x02
#define FS_O_RDWR   (FS_O_RDONLY | FS_O_WRONLY)
#define FS_O_CREAT  0x04
#define FS_O_TRUNC  0x08
#define FS_O_APPEND 0x10

struct fs_backend {
    const char *name;
    int (*read)(u32 offset, void *buf, u32 len);
    int (*write)(u32 offset, const void *buf, u32 len);
    int (*erase)(u32 offset, u32 len);
    u32 size;
};

struct fs_stat {
    u32 size;
    u32 mtime;
    bool is_dir;
};

struct fs_file;

void fs_init(void);
int fs_mount(const struct fs_backend *backend);
int fs_register_file(const char *path, const struct fs_backend *backend,
                     u32 offset, u32 size);
struct fs_file *fs_open(const char *path, u32 flags);
int fs_read(struct fs_file *file, void *buf, u32 len);
int fs_write(struct fs_file *file, const void *buf, u32 len);
int fs_seek(struct fs_file *file, u32 offset);
int fs_close(struct fs_file *file);
int fs_stat(const char *path, struct fs_stat *stat);
struct fs_backend *fs_backend(const char *name);

#endif /* __UTILS_FS_H__ */
