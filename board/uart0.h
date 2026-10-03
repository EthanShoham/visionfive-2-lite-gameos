#ifndef UART0_H
#define UART0_H
#include "../common/types.h"

void uart0_hw_init(void);
void uart0_putc(char c);
void uart0_puts(const char *s);

// Maybe move to different file
void print_hex8(u8 v);
void print_hex32(u32 v);
void print_hex64(u64 v);
void dump_bytes(const char *label, const u8 *buf, u32 len);

#endif
