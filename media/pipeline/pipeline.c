#include <media/pipeline.h>
#include <core/mem.h>
#include <core/errno.h>
#include <core/string.h>

int pipeline_init(struct pipeline *pipe)
{
    if (!pipe)
        return -EINVAL;

    INIT_LIST_HEAD(&pipe->stages);
    pipe->stage_count = 0;
    pipe->running = false;
    return 0;
}

int pipeline_add_stage(struct pipeline *pipe, const char *name,
                       stage_process_t process, void *ctx)
{
    struct pipeline_stage *stage;

    if (!pipe || !process || pipe->stage_count >= PIPELINE_MAX_STAGES)
        return -ENOSPC;

    stage = kmalloc(sizeof(*stage));
    if (!stage)
        return -ENOMEM;

    stage->name = name;
    stage->process = process;
    stage->ctx = ctx;
    list_add_tail(&stage->node, &pipe->stages);
    pipe->stage_count++;
    return 0;
}

int pipeline_run(struct pipeline *pipe, const struct audio_buffer *in,
                 struct audio_buffer *out)
{
    struct audio_buffer scratch[2];
    const struct audio_buffer *current = in;
    struct audio_buffer *next;
    struct pipeline_stage *stage;
    unsigned int index = 0;

    if (!pipe || !in || !out)
        return -EINVAL;
    if (list_empty(&pipe->stages)) {
        *out = *in;
        return 0;
    }

    list_for_each_entry(stage, &pipe->stages, node) {
        /* Alternate between the caller's output and one scratch buffer. */
        next = (index == pipe->stage_count - 1) ? out : &scratch[index & 1];
        int ret = stage->process(stage->ctx, current, next);
        if (ret)
            return ret;

        current = next;
        index++;
    }
    return 0;
}

void pipeline_clear(struct pipeline *pipe)
{
    struct pipeline_stage *stage, *tmp;

    if (!pipe)
        return;

    list_for_each_entry_safe(stage, tmp, &pipe->stages, node) {
        list_del_init(&stage->node);
        kfree(stage);
    }
    pipe->stage_count = 0;
    pipe->running = false;
}
