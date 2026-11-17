#include <driver/uart.h>
#include <core/ringbuf.h>
#include <core/string.h>
#include <core/printk.h>
#include <system/task.h>
#include <system/timer.h>

/*
 * UART core with independent RX/TX rings.
 *
 * The controller interrupt pushes bytes into the RX ring; uart_read() drains
 * it and sleeps when empty instead of busy waiting.  Writes fill the TX ring
 * and are flushed in one controller call so a long log line cannot interleave
 * with another task's output.
 */

#define UART_MAX_PORTS 4
#define UART_RING_SIZE 256

struct uart_entry {
    struct uart_port *port;
    const struct uart_ops *ops;
    bool open;
    struct ringbuf rx;
    struct ringbuf tx;
    u8 rx_storage[UART_RING_SIZE];
    u8 tx_storage[UART_RING_SIZE];
};

static struct uart_entry entries[UART_MAX_PORTS];
static unsigned int entry_count;

static struct uart_entry *find(unsigned int id)
{
    for (unsigned int i = 0; i < entry_count; i++) {
        if (entries[i].port->id == id)
            return &entries[i];
    }
    return NULL;
}

int uart_register(struct uart_port *port, const struct uart_ops *ops)
{
    struct uart_entry *entry;

    if (!port || !ops || !ops->write || entry_count >= UART_MAX_PORTS)
        return -EINVAL;

    entry = &entries[entry_count++];
    entry->port = port;
    entry->ops = ops;
    entry->open = false;
    ringbuf_init(&entry->rx, entry->rx_storage, sizeof(entry->rx_storage));
    ringbuf_init(&entry->tx, entry->tx_storage, sizeof(entry->tx_storage));
    return 0;
}

struct uart_port *uart_get(unsigned int id)
{
    struct uart_entry *e = find(id);
    return e ? e->port : NULL;
}

int uart_open(unsigned int id, u32 baudrate)
{
    struct uart_entry *e = find(id);

    if (!e)
        return -ENODEV;
    if (e->open)
        return -EBUSY;

    if (e->ops->init) {
        int ret = e->ops->init(e->port, baudrate);
        if (ret)
            return ret;
    }

    ringbuf_reset(&e->rx);
    ringbuf_reset(&e->tx);
    e->port->baudrate = baudrate;
    e->open = true;
    return 0;
}

int uart_close(unsigned int id)
{
    struct uart_entry *e = find(id);

    if (!e || !e->open)
        return -ENODEV;

    uart_flush(id);
    e->open = false;
    return 0;
}

int uart_write(unsigned int id, const void *data, size_t len)
{
    struct uart_entry *e = find(id);
    const u8 *src = data;
    size_t written = 0;

    if (!e || !e->open)
        return -ENODEV;

    while (written < len) {
        u32 space = ringbuf_free(&e->tx);
        u32 chunk = (u32)(len - written);

        if (chunk > space)
            chunk = space;
        if (chunk) {
            ringbuf_write(&e->tx, src + written, chunk);
            written += chunk;
        }

        if (ringbuf_free(&e->tx) < 32) {
            const u8 *out;
            u32 contig = ringbuf_read_ptr(&e->tx, &out);
            if (contig) {
                e->ops->write(e->port, out, contig);
                ringbuf_read_put(&e->tx, contig);
            }
        }
    }
    return (int)written;
}

void uart_input(struct uart_port *port, const u8 *data, size_t len)
{
    struct uart_entry *e = find(port->id);

    if (!e)
        return;
    ringbuf_write(&e->rx, data, (u32)len);
}

u32 uart_available(unsigned int id)
{
    struct uart_entry *e = find(id);
    return e ? ringbuf_used(&e->rx) : 0;
}

int uart_read_byte(unsigned int id, u8 *c)
{
    struct uart_entry *e = find(id);

    if (!e || !c)
        return -EINVAL;
    return ringbuf_read(&e->rx, c, 1) == 1 ? 0 : -EAGAIN;
}

int uart_read(unsigned int id, void *data, size_t len, u32 timeout_ms)
{
    struct uart_entry *e = find(id);
    u8 *dst = data;
    u32 start = 0;
    size_t got = 0;

    if (!e || !e->open)
        return -ENODEV;

    while (got < len) {
        got += ringbuf_read(&e->rx, dst + got, (u32)(len - got));
        if (got == len)
            break;

        if (timeout_ms == UART_NO_TIMEOUT)
            continue;

        if (!start)
            start = system_ticks();
        if (system_ticks() - start >= timeout_ms)
            break;

        task_sleep(1);
    }
    return (int)got;
}

int uart_flush(unsigned int id)
{
    struct uart_entry *e = find(id);

    if (!e || !e->open || !e->ops->flush)
        return -ENODEV;

    while (!ringbuf_empty(&e->tx)) {
        const u8 *out;
        u32 contig = ringbuf_read_ptr(&e->tx, &out);
        if (!contig)
            break;
        e->ops->write(e->port, out, contig);
        ringbuf_read_put(&e->tx, contig);
    }
    return e->ops->flush(e->port);
}

int uart_putc(unsigned int id, char c)
{
    return uart_write(id, &c, 1);
}

int uart_puts(unsigned int id, const char *s)
{
    return uart_write(id, s, strlen(s));
}
