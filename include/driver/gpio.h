#ifndef __DRIVER_GPIO_H__
#define __DRIVER_GPIO_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Controller independent GPIO interface.
 *
 * A SoC port registers one gpio_controller; board code then refers to pins
 * by a flat id (port * 32 + pin) and never touches registers directly.
 */

enum gpio_dir {
    GPIO_DIR_IN = 0,
    GPIO_DIR_OUT,
};

enum gpio_level {
    GPIO_LOW = 0,
    GPIO_HIGH,
};

enum gpio_pull {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
};

enum gpio_irq_edge {
    GPIO_IRQ_RISING = 0,
    GPIO_IRQ_FALLING,
    GPIO_IRQ_BOTH,
    GPIO_IRQ_HIGH,
    GPIO_IRQ_LOW,
};

struct gpio_controller {
    const char *name;
    unsigned int ngpio;

    int (*set_dir)(unsigned int id, enum gpio_dir dir);
    int (*get_dir)(unsigned int id, enum gpio_dir *dir);
    int (*set_level)(unsigned int id, enum gpio_level level);
    int (*get_level)(unsigned int id);
    int (*set_pull)(unsigned int id, enum gpio_pull pull);
    int (*set_irq)(unsigned int id, enum gpio_irq_edge edge);
    int (*irq_enable)(unsigned int id, bool enable);
};

int gpio_register(struct gpio_controller *ctrl);
struct gpio_controller *gpio_lookup(const char *name);

int gpio_set_dir(unsigned int id, enum gpio_dir dir);
int gpio_get_dir(unsigned int id, enum gpio_dir *dir);
int gpio_set_level(unsigned int id, enum gpio_level level);
int gpio_get_level(unsigned int id);
int gpio_set_pull(unsigned int id, enum gpio_pull pull);
int gpio_set_irq(unsigned int id, enum gpio_irq_edge edge);
int gpio_irq_enable(unsigned int id, bool enable);
int gpio_toggle(unsigned int id);

/* Debounced input with edge events. */
typedef void (*gpio_event_cb_t)(unsigned int id, enum gpio_level level, void *arg);
int gpio_debounce(unsigned int id, u16 ms, gpio_event_cb_t cb, void *arg);
void gpio_scan(u32 now_ms);

static inline int gpio_output(unsigned int id, enum gpio_level level)
{
    int ret = gpio_set_dir(id, GPIO_DIR_OUT);
    return ret ? ret : gpio_set_level(id, level);
}

static inline int gpio_input(unsigned int id, enum gpio_pull pull)
{
    int ret = gpio_set_pull(id, pull);
    return ret ? ret : gpio_set_dir(id, GPIO_DIR_IN);
}

#endif /* __DRIVER_GPIO_H__ */
