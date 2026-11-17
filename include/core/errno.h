#ifndef __CORE_ERRNO_H__
#define __CORE_ERRNO_H__

/*
 * Negative errno values travel through the driver and media layers; a return
 * value of 0 means success and any negative value identifies the fault.
 */
#define EPERM    1
#define ENOENT   2
#define EIO      3
#define ENXIO    4
#define EAGAIN   5
#define ENOMEM   6
#define EACCES   7
#define EBUSY    8
#define EEXIST   9
#define ENODEV   10
#define EINVAL   11
#define ENOSPC   12
#define ERANGE   13
#define ENOSYS   14
#define ETIMEDOUT 15
#define ECANCELED 16
#define EALREADY  19
#define ESHUTDOWN 17
#define EPROTO   18

#define ERR_PTR(v)  ((void *)(sptr)(v))
#define PTR_ERR(p)  ((s32)(sptr)(p))
#define IS_ERR(p)   ((uptr)(p) >= (uptr)-4095)
#define IS_ERR_OR_NULL(p) ((p) == NULL || IS_ERR(p))
#define PTR_ERR_OR_ZERO(p) (IS_ERR(p) ? PTR_ERR(p) : 0)

#endif /* __CORE_ERRNO_H__ */
