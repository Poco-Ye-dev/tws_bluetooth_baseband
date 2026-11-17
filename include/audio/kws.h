#ifndef __AUDIO_KWS_H__
#define __AUDIO_KWS_H__

#include <core/types.h>
#include <core/errno.h>
#include <media/buffer.h>

/*
 * Keyword spotting.  The model runs continuously on the microphone frame;
 * when a keyword is recognised the callback fires from the audio task.
 */

#define KWS_MAX_KEYWORDS 8
#define KWS_KEYWORD_MAX 24

typedef void (*kws_cb_t)(u32 keyword_id, void *arg);

struct kws_keyword {
    char text[KWS_KEYWORD_MAX];
    u8 threshold;
};

struct kws {
    struct kws_keyword keyword[KWS_MAX_KEYWORDS];
    u8 count;
    bool running;
    kws_cb_t callback;
    void *arg;
    void *model;
};

int kws_init(struct kws *kws, kws_cb_t cb, void *arg);
int kws_add_keyword(struct kws *kws, const char *text, u8 threshold);
int kws_start(struct kws *kws);
int kws_stop(struct kws *kws);
int kws_process(struct kws *kws, const struct audio_buffer *frame);
void kws_deinit(struct kws *kws);

#endif /* __AUDIO_KWS_H__ */
