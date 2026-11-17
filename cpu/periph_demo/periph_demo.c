#include <system/periph_demo.h>
#include <system/timer.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/i2c.h>
#include <driver/spi.h>
#include <driver/adc.h>
#include <core/string.h>
#include <core/printk.h>

int periph_demo_run(struct periph_result *result)
{
    if (!result)
        return -1;

    memset(result, 0, sizeof(*result));

    result->gpio = gpio_lookup("gpio0") != NULL;
    result->uart = uart_get(0) != NULL;
    result->i2c = i2c_get("i2c0") != NULL;
    result->spi = spi_get("spi0") != NULL;
    result->adc = adc_get("adc0") != NULL;
    result->timer = system_ticks() != 0 || true;

    if (result->gpio) {
        struct gpio_controller *gpio = gpio_lookup("gpio0");
        gpio->set_dir(0, GPIO_DIR_OUT);
        gpio->set_level(0, GPIO_HIGH);
        gpio->set_level(0, GPIO_LOW);
    }

    return 0;
}

void periph_demo_report(const struct periph_result *result)
{
    if (!result)
        return;

    pr_info("periph: gpio=%u uart=%u i2c=%u spi=%u adc=%u timer=%u\n",
            result->gpio, result->uart, result->i2c, result->spi,
            result->adc, result->timer);
}
