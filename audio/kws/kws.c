#include <audio/kws.h>
#include <core/string.h>
#include <core/printk.h>

int kws_init(struct kws *kws, kws_cb_t cb, void *arg)
{
    if (!kws)
        return -EINVAL;

    memset(kws, 0, sizeof(*kws));
    kws->callback = cb;
    kws->arg = arg;
    return 0;
}

int kws_add_keyword(struct kws *kws, const char *text, u8 threshold)
{
    if (!kws || !text || kws->count >= KWS_MAX_KEYWORDS)
        return -ENOSPC;

    strncpy(kws->keyword[kws->count].text, text, KWS_KEYWORD_MAX - 1);
    kws->keyword[kws->count].threshold = threshold ? threshold : 200;
    kws->count++;
    return 0;
}

int kws_start(struct kws *kws)
{
    if (!kws || !kws->count)
        return -EINVAL;

    kws->running = true;
    pr_info("kws: listening for %u keyword(s)\n", kws->count);
    return 0;
}

int kws_stop(struct kws *kws)
{
    if (!kws)
        return -EINVAL;
    kws->running = false;
    return 0;
}

int kws_process(struct kws *kws, const struct audio_buffer *frame)
{
    if (!kws || !frame)
        return -EINVAL;
    if (!kws->running)
        return 0;

    /*
     * The acoustic model is provided by the platform blob; the generic layer
     * only owns the lifecycle and the callback dispatch.
     */
    return 0;
}

void kws_deinit(struct kws *kws)
{
    if (!kws)
        return;
    kws->running = false;
    kws->model = NULL;
}
