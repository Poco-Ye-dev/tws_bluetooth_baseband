#include <utils/fs.h>
#include <core/string.h>
#include <core/errno.h>
#include <core/printk.h>

struct fs_entry {
    bool used;
    char path[FS_PATH_MAX];
    const struct fs_backend *backend;
    u32 offset;
    u32 size;
};

struct fs_file {
    bool used;
    struct fs_entry *entry;
    u32 offset;
    u32 flags;
};

static struct fs_entry entries[FS_MAX_FILES];
static struct fs_file files[FS_MAX_FILES];
static const struct fs_backend *backends[FS_MAX_BACKENDS];
static unsigned int backend_count;

void fs_init(void)
{
    memset(entries, 0, sizeof(entries));
    memset(files, 0, sizeof(files));
    memset(backends, 0, sizeof(backends));
    backend_count = 0;
}

int fs_mount(const struct fs_backend *backend)
{
    if (!backend || !backend->read || backend_count >= FS_MAX_BACKENDS)
        return -EINVAL;

    for (unsigned int i = 0; i < backend_count; i++) {
        if (strcmp(backends[i]->name, backend->name) == 0)
            return -EEXIST;
    }

    backends[backend_count++] = backend;
    return 0;
}

struct fs_backend *fs_backend(const char *name)
{
    for (unsigned int i = 0; i < backend_count; i++) {
        if (strcmp(backends[i]->name, name) == 0)
            return (struct fs_backend *)backends[i];
    }
    return NULL;
}

int fs_register_file(const char *path, const struct fs_backend *backend,
                     u32 offset, u32 size)
{
    if (!path || !backend)
        return -EINVAL;
    if (backend->size && offset + size > backend->size)
        return -ERANGE;

    for (unsigned int i = 0; i < FS_MAX_FILES; i++) {
        if (entries[i].used)
            continue;
        entries[i].used = true;
        strncpy(entries[i].path, path, FS_PATH_MAX - 1);
        entries[i].backend = backend;
        entries[i].offset = offset;
        entries[i].size = size;
        return 0;
    }
    return -ENOSPC;
}

static struct fs_entry *lookup(const char *path)
{
    for (unsigned int i = 0; i < FS_MAX_FILES; i++) {
        if (entries[i].used && strcmp(entries[i].path, path) == 0)
            return &entries[i];
    }
    return NULL;
}

struct fs_file *fs_open(const char *path, u32 flags)
{
    struct fs_entry *entry = lookup(path);

    if (!entry)
        return NULL;

    for (unsigned int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used)
            continue;
        files[i].used = true;
        files[i].entry = entry;
        files[i].offset = (flags & FS_O_APPEND) ? entry->size : 0;
        files[i].flags = flags;
        return &files[i];
    }
    return NULL;
}

int fs_read(struct fs_file *file, void *buf, u32 len)
{
    u32 available;

    if (!file || !file->used || !(file->flags & FS_O_RDONLY))
        return -EINVAL;

    available = file->entry->size - file->offset;
    if (len > available)
        len = available;
    if (!len)
        return 0;

    int ret = file->entry->backend->read(file->entry->offset + file->offset,
                                          buf, len);
    if (ret < 0)
        return ret;

    file->offset += len;
    return (int)len;
}

int fs_write(struct fs_file *file, const void *buf, u32 len)
{
    if (!file || !file->used || !(file->flags & FS_O_WRONLY))
        return -EINVAL;
    if (!file->entry->backend->write)
        return -EPERM;

    if (file->offset + len > file->entry->size)
        return -ENOSPC;

    int ret = file->entry->backend->write(file->entry->offset + file->offset,
                                           buf, len);
    if (ret < 0)
        return ret;

    file->offset += len;
    return (int)len;
}

int fs_seek(struct fs_file *file, u32 offset)
{
    if (!file || !file->used)
        return -EINVAL;
    if (offset > file->entry->size)
        return -ERANGE;
    file->offset = offset;
    return 0;
}

int fs_close(struct fs_file *file)
{
    if (!file || !file->used)
        return -EINVAL;
    memset(file, 0, sizeof(*file));
    return 0;
}

int fs_stat(const char *path, struct fs_stat *stat)
{
    struct fs_entry *entry = lookup(path);

    if (!entry || !stat)
        return -ENOENT;

    stat->size = entry->size;
    stat->mtime = 0;
    stat->is_dir = false;
    return 0;
}
