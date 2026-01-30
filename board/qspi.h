#ifndef __QSPI_H__
#define __QSPI_H__
#include "../common/types.h"

void qspi_init(void);
void qspi_indirect_read(u32 addr, u8 *dst, u32 len);

// Return pointer to QSPI AHB window (memory-mapped flash).
static inline u8 *qspi_ahb_base(void) { return (u8 *)0x21000000u; }

#endif /* __QSPI_H__ */
