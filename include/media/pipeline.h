#ifndef __MEDIA_PIPELINE_H__
#define __MEDIA_PIPELINE_H__

#include <core/types.h>
#include <core/list.h>
#include <media/buffer.h>

/*
 * A pipeline is a chain of stages, each consuming one buffer and producing
 * one buffer.  Effects are stages; codecs sit at the ends.
 */

#define PIPELINE_MAX_STAGES 8

typedef int (*stage_process_t)(void *ctx, const struct audio_buffer *in,
                               struct audio_buffer *out);

struct pipeline_stage {
    struct list_head node;
    const char *name;
    stage_process_t process;
    void *ctx;
};

struct pipeline {
    struct list_head stages;
    u32 stage_count;
    bool running;
};

int pipeline_init(struct pipeline *pipe);
int pipeline_add_stage(struct pipeline *pipe, const char *name,
                       stage_process_t process, void *ctx);
int pipeline_run(struct pipeline *pipe, const struct audio_buffer *in,
                 struct audio_buffer *out);
void pipeline_clear(struct pipeline *pipe);

#endif /* __MEDIA_PIPELINE_H__ */
