#include <core/printk.h>
#include <core/assert.h>
#include <core/string.h>
#include <core/log.h>
#include <stdarg.h>

static char log_line[96];
static u32 log_len;

/*
 * Minimal formatted output.  Only the conversions the firmware actually uses
 * are implemented; anything else is printed verbatim so a missing case is
 * visible instead of silently corrupting the line.
 */

__weak void platform_putc(char c)
{
    (void)c;
}

__weak __noreturn void platform_halt(void)
{
    for (;;)
        ;
}

static void emit(char c)
{
    if (c == '\n') {
        log_line[log_len] = '\0';
        log_write(0, log_line);
        log_len = 0;
        platform_putc('\r');
    } else if (log_len < sizeof(log_line) - 1) {
        log_line[log_len++] = c;
    }
    platform_putc(c);
}

static void emit_padding(char pad, int count)
{
    while (count-- > 0)
        emit(pad);
}

static int utoa(char *buf, u32 value, int base, bool upper)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    const char *digits = upper ? upper_digits : lower_digits;
    char tmp[16];
    int n = 0;

    do {
        tmp[n++] = digits[value % (u32)base];
        value /= (u32)base;
    } while (value);

    for (int i = 0; i < n; i++)
        buf[i] = tmp[n - 1 - i];
    return n;
}

static void format_one(char conv, u32 value, int width, char pad, bool upper)
{
    char buf[16];
    int len;

    if (conv == 'c') {
        buf[0] = (char)value;
        len = 1;
    } else if (conv == 's') {
        const char *s = (const char *)(uptr)value;
        if (!s)
            s = "(null)";
        for (const char *p = s; *p; p++)
            emit(*p);
        return;
    } else if (conv == 'p') {
        emit('0');
        emit('x');
        len = utoa(buf, value, 16, false);
    } else if (conv == 'x') {
        len = utoa(buf, value, 16, upper);
    } else if (conv == 'u') {
        len = utoa(buf, value, 10, false);
    } else {
        len = utoa(buf, value, 10, false);
    }

    emit_padding(pad, width - len);
    for (int i = 0; i < len; i++)
        emit(buf[i]);
}

void vprintk(const char *fmt, __builtin_va_list ap)
{
    const char *p = fmt;

    while (*p) {
        int width = 0;
        char pad = ' ';
        bool long_arg = false;
        bool upper = false;

        if (*p != '%') {
            emit(*p++);
            continue;
        }

        p++;
        if (*p == '%') {
            emit('%');
            p++;
            continue;
        }

        if (*p == '0') {
            pad = '0';
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }
        while (*p == 'l') {
            long_arg = true;
            p++;
        }

        char conv = *p++;
        switch (conv) {
        case 'd':
        case 'i': {
            s32 v = long_arg ? (s32)va_arg(ap, long) : va_arg(ap, int);
            if (v < 0) {
                emit('-');
                format_one('u', (u32)(-v), width > 0 ? width - 1 : 0, pad, false);
            } else {
                format_one('u', (u32)v, width, pad, false);
            }
            break;
        }
        case 'u':
            format_one('u', long_arg ? (u32)va_arg(ap, unsigned long)
                                     : va_arg(ap, unsigned int),
                       width, pad, false);
            break;
        case 'x':
        case 'X':
            upper = (conv == 'X');
            format_one('x', long_arg ? (u32)va_arg(ap, unsigned long)
                                     : va_arg(ap, unsigned int),
                       width, pad, upper);
            break;
        case 'p':
            format_one('p', (u32)(uptr)va_arg(ap, void *), width, pad, false);
            break;
        case 'c':
            format_one('c', (u32)va_arg(ap, int), width, pad, false);
            break;
        case 's':
            format_one('s', (u32)(uptr)va_arg(ap, const char *), width, pad, false);
            break;
        default:
            emit('%');
            if (conv)
                emit(conv);
            break;
        }
    }
}

void printk(const char *fmt, ...)
{
    __builtin_va_list ap;

    __builtin_va_start(ap, fmt);
    vprintk(fmt, ap);
    __builtin_va_end(ap);
}

void panic(const char *file, int line, const char *expr)
{
    printk("\nPANIC %s:%d: %s\n", file, line, expr);
    platform_halt();
}
