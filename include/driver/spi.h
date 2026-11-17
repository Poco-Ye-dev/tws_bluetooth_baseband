#ifndef __DRIVER_SPI_H__
#define __DRIVER_SPI_H__

#include <core/types.h>
#include <core/errno.h>

enum spi_mode {
    SPI_MODE_0 = 0,
    SPI_MODE_1,
    SPI_MODE_2,
    SPI_MODE_3,
};

struct spi_device {
    u8 mode;
    u32 max_hz;
    u8 bits_per_word;
    bool lsb_first;
    u32 cs;
};

struct spi_controller {
    const char *name;
    u32 clock_hz;
    int (*setup)(struct spi_controller *ctrl, struct spi_device *dev);
    int (*transfer)(struct spi_controller *ctrl, struct spi_device *dev,
                    const u8 *tx, u8 *rx, size_t len);
    void *priv;
};

int spi_register(struct spi_controller *ctrl);
struct spi_controller *spi_get(const char *name);

int spi_setup(struct spi_controller *ctrl, struct spi_device *dev);
int spi_transfer(struct spi_controller *ctrl, struct spi_device *dev,
                 const u8 *tx, u8 *rx, size_t len);
int spi_write(struct spi_controller *ctrl, struct spi_device *dev,
              const void *data, size_t len);
int spi_read(struct spi_controller *ctrl, struct spi_device *dev,
             void *data, size_t len);

#endif /* __DRIVER_SPI_H__ */
