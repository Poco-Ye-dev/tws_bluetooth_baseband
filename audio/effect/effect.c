#include <audio/effect.h>
#include <core/string.h>

#define MAX_EFFECTS 8

static const struct effect_ops *registry[MAX_EFFECTS];
static unsigned int registry_count;

int effect_register(const struct effect_ops *ops)
{
    if (!ops || !ops->open || !ops->process || registry_count >= MAX_EFFECTS)
        return -ENOSPC;

    registry[registry_count++] = ops;
    return 0;
}

const struct effect_ops *effect_find(const char *name)
{
    for (unsigned int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i]->name, name) == 0)
            return registry[i];
    }
    return NULL;
}

int effect_open(struct effect *effect, const char *name, const void *config)
{
    const struct effect_ops *ops;

    if (!effect || !name)
        return -EINVAL;

    ops = effect_find(name);
    if (!ops)
        return -ENOENT;

    effect->ctx = NULL;
    int ret = ops->open(&effect->ctx, config);
    if (ret)
        return ret;

    effect->ops = ops;
    effect->enabled = true;
    return 0;
}

int effect_process(struct effect *effect, const struct audio_buffer *in,
                   struct audio_buffer *out)
{
    if (!effect || !effect->enabled || !effect->ops)
        return -EINVAL;
    return effect->ops->process(effect->ctx, in, out);
}

void effect_enable(struct effect *effect, bool enable)
{
    if (effect)
        effect->enabled = enable;
}

void effect_set_param(struct effect *effect, u32 id, s32 value)
{
    if (effect && effect->ops && effect->ops->set_param)
        effect->ops->set_param(effect->ctx, id, value);
}

void effect_close(struct effect *effect)
{
    if (!effect || !effect->ops)
        return;
    if (effect->ops->close)
        effect->ops->close(effect->ctx);
    effect->ctx = NULL;
    effect->ops = NULL;
}
