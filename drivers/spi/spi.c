#include <driver/spi.h>
#include <core/string.h>

#define SPI_MAX_CONTROLLERS 3

static struct spi_controller *controllers[SPI_MAX_CONTROLLERS];
static unsigned int controller_count;

int spi_register(struct spi_controller *ctrl)
{
    if (!ctrl || !ctrl->transfer || controller_count >= SPI_MAX_CONTROLLERS)
        return -EINVAL;

    controllers[controller_count++] = ctrl;
    return 0;
}

struct spi_controller *spi_get(const char *name)
{
    for (unsigned int i = 0; i < controller_count; i++) {
        if (strcmp(controllers[i]->name, name) == 0)
            return controllers[i];
    }
    return NULL;
}

int spi_setup(struct spi_controller *ctrl, struct spi_device *dev)
{
    if (!ctrl || !dev)
        return -EINVAL;
    if (dev->bits_per_word == 0)
        dev->bits_per_word = 8;
    if (!ctrl->setup)
        return 0;
    return ctrl->setup(ctrl, dev);
}

int spi_transfer(struct spi_controller *ctrl, struct spi_device *dev,
                 const u8 *tx, u8 *rx, size_t len)
{
    if (!ctrl || !ctrl->transfer || !dev)
        return -EINVAL;
    return ctrl->transfer(ctrl, dev, tx, rx, len);
}

int spi_write(struct spi_controller *ctrl, struct spi_device *dev,
              const void *data, size_t len)
{
    return spi_transfer(ctrl, dev, data, NULL, len);
}

int spi_read(struct spi_controller *ctrl, struct spi_device *dev,
             void *data, size_t len)
{
    return spi_transfer(ctrl, dev, NULL, data, len);
}
