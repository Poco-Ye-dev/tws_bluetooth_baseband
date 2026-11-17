#include <core/string.h>
#include <core/mem.h>

void *memset(void *dst, int c, size_t n)
{
    u8 *d = dst;
    while (n--)
        *d++ = (u8)c;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    u8 *d = dst;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    u8 *d = dst;
    const u8 *s = src;

    if (d == s || n == 0)
        return dst;

    if (d < s || d >= s + n) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const u8 *pa = a;
    const u8 *pb = b;
    while (n--) {
        if (*pa != *pb)
            return (int)*pa - (int)*pb;
        pa++;
        pb++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const u8 *p = s;
    while (n--) {
        if (*p == (u8)c)
            return (void *)p;
        p++;
    }
    return NULL;
}

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

size_t strnlen(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n])
        n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)(u8)*a - (int)(u8)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0)
        return 0;
    return (int)(u8)*a - (int)(u8)*b;
}

static int to_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

int strcasecmp(const char *a, const char *b)
{
    while (*a && to_lower((u8)*a) == to_lower((u8)*b)) {
        a++;
        b++;
    }
    return to_lower((u8)*a) - to_lower((u8)*b);
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++))
        ;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    char *d = dst;
    while (n && *src) {
        *d++ = *src++;
        n--;
    }
    while (n--)
        *d++ = '\0';
    return dst;
}

char *strcat(char *dst, const char *src)
{
    strcpy(dst + strlen(dst), src);
    return dst;
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c)
            last = s;
        s++;
    }
    if (c == '\0')
        return (char *)s;
    return (char *)last;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle &&
            strncmp(haystack, needle, strlen(needle)) == 0)
            return (char *)haystack;
    }
    return NULL;
}

char *strdup(const char *s)
{
    return strndup(s, strlen(s));
}

char *strndup(const char *s, size_t n)
{
    size_t len = strnlen(s, n);
    char *out = kmalloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

const char *parse_uint(const char *s, u32 *out, int base)
{
    u32 value = 0;
    const char *p = s;
    int digits = 0;

    if (!p || !out || (base != 10 && base != 16))
        return NULL;

    while (*p) {
        int c = (u8)*p;
        int d;

        if (c >= '0' && c <= '9')
            d = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f')
            d = c - 'a' + 10;
        else if (base == 16 && c >= 'A' && c <= 'F')
            d = c - 'A' + 10;
        else
            break;

        if (d >= base)
            break;

        value = value * (u32)base + (u32)d;
        p++;
        digits++;
    }

    if (!digits)
        return NULL;

    *out = value;
    return p;
}

const char *parse_hex(const char *s, u32 *out)
{
    if (s && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    return parse_uint(s, out, 16);
}

const char *parse_int(const char *s, s32 *out)
{
    bool negative = false;
    const char *p = s;
    u32 magnitude;

    if (!p || !out)
        return NULL;

    if (*p == '-' || *p == '+') {
        negative = (*p == '-');
        p++;
    }

    p = parse_uint(p, &magnitude, 10);
    if (!p)
        return NULL;

    *out = negative ? -(s32)magnitude : (s32)magnitude;
    return p;
}
