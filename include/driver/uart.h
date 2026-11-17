#ifndef __DRIVER_UART_H__
#define __DRIVER_UART_H__

#include <core/types.h>
#include <core/errno.h>

/*
 * Polled or interrupt driven UART.  The board layer opens a port with a
 * baud rate and then uses the byte oriented API; DMA is an implementation
 * detail of the controller.
 */

#define UART_NO_TIMEOUT 0

struct uart_port {
    const char *name;
    unsigned int id;
    u32 baudrate;
    void *priv;
};

struct uart_ops {
    int (*init)(struct uart_port *port, u32 baudrate);
    int (*write)(struct uart_port *port, const u8 *data, size_t len);
    int (*read)(struct uart_port *port, u8 *data, size_t len, u32 timeout_ms);
    int (*flush)(struct uart_port *port);
};

int uart_register(struct uart_port *port, const struct uart_ops *ops);

struct uart_port *uart_get(unsigned int id);
int uart_open(unsigned int id, u32 baudrate);
int uart_close(unsigned int id);
int uart_write(unsigned int id, const void *data, size_t len);
int uart_read(unsigned int id, void *data, size_t len, u32 timeout_ms);
int uart_putc(unsigned int id, char c);
int uart_puts(unsigned int id, const char *s);
int uart_flush(unsigned int id);

/* Feed received bytes from the interrupt handler into the RX ring. */
void uart_input(struct uart_port *port, const u8 *data, size_t len);
u32 uart_available(unsigned int id);
int uart_read_byte(unsigned int id, u8 *c);

#endif /* __DRIVER_UART_H__ */
