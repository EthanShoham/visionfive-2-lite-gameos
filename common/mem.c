#include "types.h"

__attribute__((used))
void *memcpy(void *dst, const void *src, u64 n)
{
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dst;
}

__attribute__((used))
void *memset(void *dst, int c, u64 n)
{
    u8 *d = (u8*)dst;
    u8 v = (u8)c;
    while (n--) *d++ = v;
    return dst;
}

__attribute__((used))
void *memmove(void *dst, const void *src, u64 n)
{
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}
