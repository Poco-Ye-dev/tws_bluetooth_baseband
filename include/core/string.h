#ifndef __CORE_STRING_H__
#define __CORE_STRING_H__

#include <core/types.h>

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
void *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t max);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
int    strcasecmp(const char *a, const char *b);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);

char  *strdup(const char *s);
char  *strndup(const char *s, size_t n);

/*
 * Parse a decimal integer.  Returns a pointer past the last consumed
 * character, or NULL when no digits were found.
 */
const char *parse_int(const char *s, s32 *out);
const char *parse_uint(const char *s, u32 *out, int base);
const char *parse_hex(const char *s, u32 *out);

#endif /* __CORE_STRING_H__ */
