#include <driver/gpio.h>
#include <core/string.h>
#include <core/printk.h>
#include <system/timer.h>

/*
 * GPIO core.
 *
 * Besides dispatch, this layer owns debouncing and edge events.  A pin that
 * has a handler is sampled by the 1 kHz tick; the callback only fires after
 * the level has been stable for the configured interval.
 */

#define GPIO_MAX_CONTROLLERS 4
#define GPIO_MAX_PINS 128

struct pin_state {
    u8 level;
    u8 stable;
    u16 debounce_ms;
    u16 elapsed_ms;
    bool used;
    gpio_event_cb_t cb;
    void *arg;
};

static struct gpio_controller *controllers[GPIO_MAX_CONTROLLERS];
static unsigned int controller_count;
static struct pin_state pins[GPIO_MAX_PINS];
static bool gpio_scan_registered;

static struct gpio_controller *controller_for(unsigned int id,
                                               unsigned int *local)
{
    unsigned int base = 0;

    for (unsigned int i = 0; i < controller_count; i++) {
        struct gpio_controller *ctrl = controllers[i];
        if (id < base + ctrl->ngpio) {
            *local = id - base;
            return ctrl;
        }
        base += ctrl->ngpio;
    }
    return NULL;
}

int gpio_register(struct gpio_controller *ctrl)
{
    if (!ctrl || !ctrl->ngpio || controller_count >= GPIO_MAX_CONTROLLERS)
        return -EINVAL;

    controllers[controller_count++] = ctrl;
    pr_info("gpio: registered %s (%u pins)\n", ctrl->name, ctrl->ngpio);
    return 0;
}

struct gpio_controller *gpio_lookup(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

int gpio_set_dir(unsigned int id, enum gpio_dir dir)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->set_dir)
        return -ENODEV;
    return ctrl->set_dir(local, dir);
}

int gpio_get_dir(unsigned int id, enum gpio_dir *dir)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->get_dir)
        return -ENODEV;
    return ctrl->get_dir(local, dir);
}

int gpio_set_level(unsigned int id, enum gpio_level level)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->set_level)
        return -ENODEV;
    return ctrl->set_level(local, level);
}

int gpio_get_level(unsigned int id)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->get_level)
        return -ENODEV;
    return ctrl->get_level(local);
}

int gpio_toggle(unsigned int id)
{
    int level = gpio_get_level(id);

    if (level < 0)
        return level;
    return gpio_set_level(id, level ? GPIO_LOW : GPIO_HIGH);
}

int gpio_set_pull(unsigned int id, enum gpio_pull pull)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->set_pull)
        return -ENODEV;
    return ctrl->set_pull(local, pull);
}

int gpio_set_irq(unsigned int id, enum gpio_irq_edge edge)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->set_irq)
        return -ENODEV;
    return ctrl->set_irq(local, edge);
}

int gpio_irq_enable(unsigned int id, bool enable)
{
    unsigned int local;
    struct gpio_controller *ctrl = controller_for(id, &local);

    if (!ctrl || !ctrl->irq_enable)
        return -ENODEV;
    return ctrl->irq_enable(local, enable);
}

int gpio_debounce(unsigned int id, u16 ms, gpio_event_cb_t cb, void *arg)
{
    if (id >= GPIO_MAX_PINS)
        return -EINVAL;

    pins[id].used = true;
    pins[id].debounce_ms = ms;
    pins[id].cb = cb;
    pins[id].arg = arg;
    pins[id].elapsed_ms = ms;
    pins[id].stable = (u8)gpio_get_level(id);
    pins[id].level = pins[id].stable;

    if (!gpio_scan_registered) {
        gpio_scan_registered = true;
    }
    return 0;
}

void gpio_scan(u32 now_ms)
{
    (void)now_ms;

    for (unsigned int id = 0; id < GPIO_MAX_PINS; id++) {
        struct pin_state *pin = &pins[id];
        int level;

        if (!pin->used)
            continue;

        level = gpio_get_level(id);
        if (level < 0)
            continue;

        if ((u8)level == pin->stable) {
            pin->elapsed_ms = 0;
            continue;
        }

        pin->elapsed_ms++;
        if (pin->elapsed_ms < pin->debounce_ms)
            continue;

        pin->stable = (u8)level;
        pin->elapsed_ms = 0;
        if (pin->cb)
            pin->cb(id, (enum gpio_level)pin->stable, pin->arg);
    }
}
