#include "../board/ack_led.h"
#include "../board/board_init.h"
#include "../board/qspi.h"
#include "../board/uart0.h"
#include "../common/types.h"

void panic(void) {
  ack_led_off();
  for (;;)
    ;
}

void delay(u32 millsec) {
  // assuming core clock at 1GHz
  volatile u64 clocks = (1ull << 20) * millsec;
  while (clocks--)
    ;
}

#define GiB(x) ((u64)(x) << 30)

#define DDR_BASE 0x40000000u
#define DDR_SIZE GiB(8)
#define BOOT_START 0x08000000u
#define FLASH_SPL_HDR_SIZE 0x400u

// Linker-provided symbol (boot.ld): end of boot.bin in flash
extern u8 boot_end[];

b32 ddr_sanity_test(void) {
  volatile u32 *mem = (volatile u32 *)DDR_BASE;
  const u32 count = 256;
  const u32 pat1 = 0xA5A5A5A5u;
  const u32 pat2 = 0x5A5A5A5Au;

  for (u32 i = 0; i < count; i++) {
    mem[i] = pat1 ^ i;
  }
  for (u32 i = 0; i < count; i++) {
    if (mem[i] != (pat1 ^ i)) {
      return false;
    }
  }

  for (u32 i = 0; i < count; i++) {
    mem[i] = pat2 ^ (i * 3u);
  }
  for (u32 i = 0; i < count; i++) {
    if (mem[i] != (pat2 ^ (i * 3u))) {
      return false;
    }
  }

  return true;
}

void make_ddr_e2(void) {
  volatile u32 *mem = (volatile u32 *)DDR_BASE;
  const u64 count = DDR_SIZE;
  const u32 pat = 0xE2E2E2E2u;

  for (u32 i = 0; i < count; i++) {
    mem[i] = pat;
  }
}

static inline u64 hartid(void) {
  u64 id;
  __asm__ volatile("csrr %0, mhartid" : "=r"(id));
  return id;
}

static inline u64 load_u64_unaligned(const u8 *p) {
  u64 v = 0;
  for (u32 i = 0; i < 8u; i++) {
    v |= ((u64)p[i]) << (i * 8u);
  }
  return v;
}

extern volatile u64 handoff_supervisor;
extern volatile u64 handoff_kernel;

// this only runs on hart0 the S7 core
void boot_start(void) {
  // first indicator for good
  ack_led_init();
  ack_led_on();

  uart0_hw_init();
  uart0_puts("\nBoot starting!\n");

  boardinit();

  if (ddr_sanity_test()) {
    uart0_puts("DDR test passed!\n");
  } else {
    uart0_puts("DDR test faild!\n");
    panic();
  }

  // #ifdef DEBUG
  //   uart0_puts("Starting DDR E2 pattern\n");
  //   make_ddr_e2();
  //   uart0_puts("Finished DDR E2 pattern\n");
  // #endif

  // init QSPI for memory-mapped reads
  qspi_init();

  // copy kernel from QSPI flash to DDR
  uart0_puts("start copying code to DDR!\n");
  uptr kernel_start;
  uptr supervisor_start;
  {
    const u64 header_smagic = 0x6D8CF37B26F312B4ull; // "SUPERVISORHDR1"
    const u64 header_kmagic = 0x4B524E4C48445231ull; // "KRNLHDR1"
    u64 boot_size = (u64)(boot_end - (u8 *)BOOT_START);
    u8 *hdr = qspi_ahb_base() + FLASH_SPL_HDR_SIZE + boot_size;
    u64 smagic = ((u64 *)hdr)[0];
    u64 ssize = ((u64 *)hdr)[1];
    u64 kmagic = ((u64 *)hdr)[2];
    u64 ksize = ((u64 *)hdr)[3];

    uart0_puts("Supervisor header magic: ");
    print_hex64(smagic);
    uart0_puts("\n");

    uart0_puts("Supervisor header size: ");
    print_hex64(ssize);
    uart0_puts("\n");

    uart0_puts("Kernel header magic: ");
    print_hex64(kmagic);
    uart0_puts("\n");

    uart0_puts("Kernel header size: ");
    print_hex64(ksize);
    uart0_puts("\n");

    if (smagic != header_smagic) {
      uart0_puts("Supervisor header magic mismatch!\n");
      panic();
    }

    if (kmagic != header_kmagic) {
      uart0_puts("Kernel header magic mismatch!\n");
      panic();
    }

    u8 *src8 = hdr + 32u;
    uart0_puts("From src: ");
    print_hex64((u64)src8);
    uart0_puts("\n");
    u8 *dst8 = (u8 *)DDR_BASE;
    uart0_puts("To dest: ");
    print_hex64((u64)dst8);
    uart0_puts("\n");
    u8 *src_end = src8 + ssize + ksize;
    supervisor_start = (uptr)dst8;
    kernel_start = (uptr)(dst8 + ssize);

    // Align to 8 bytes before bulk copy.
    while (((uptr)src8 & 7u) && (src8 < src_end)) {
      *dst8++ = *src8++;
    }

    // Copy 64-bit chunks.
    u64 *src64 = (u64 *)src8;
    u64 *dst64 = (u64 *)dst8;
    u64 *end64 = (u64 *)((uptr)src_end & ~7u);
    while (src64 < end64) {
      *dst64++ = *src64++;
    }

    // Copy any remaining tail bytes.
    src8 = (u8 *)src64;
    dst8 = (u8 *)dst64;
    while (src8 < src_end) {
      *dst8++ = *src8++;
    }

    // Validate copy
    b32 copy_ok = true;
    u8 *verify_src8 = hdr + 32u;
    u8 *verify_dst8 = (u8 *)DDR_BASE;
    u8 *verify_end = verify_src8 + ssize + ksize;

    while (((uptr)verify_src8 & 7u) && (verify_src8 < verify_end)) {
      if (*verify_src8++ != *verify_dst8++) {
        copy_ok = false;
        break;
      }
    }

    if (copy_ok) {
      u64 *vsrc64 = (u64 *)verify_src8;
      u64 *vdst64 = (u64 *)verify_dst8;
      u64 *vend64 = (u64 *)((uptr)verify_end & ~7u);
      while (vsrc64 < vend64) {
        if (*vsrc64++ != *vdst64++) {
          copy_ok = false;
          break;
        }
      }
      verify_src8 = (u8 *)vsrc64;
      verify_dst8 = (u8 *)vdst64;
    }

    if (copy_ok) {
      while (verify_src8 < verify_end) {
        if (*verify_src8++ != *verify_dst8++) {
          copy_ok = false;
          break;
        }
      }
    }

    uart0_puts("Supervisor first 64-bit: ");
    print_hex64(load_u64_unaligned((u8 *)supervisor_start));
    uart0_puts("\n");
    uart0_puts("Kernel first 64-bit: ");
    print_hex64(load_u64_unaligned((u8 *)kernel_start));
    uart0_puts("\n");

    if (copy_ok) {
      uart0_puts("Copy validation success!\n");
    } else {
      uart0_puts("Copy validation failed!\n");
    }
  }
  uart0_puts("finished copying supervisor and kernel to DDR!\n");
  ack_led_off();
  uart0_puts("Boot finished!\n");

  // set uptr on stack to starting address of supervisor and kernel to release
  // the other cores
  uart0_puts("Supervisor start address: ");
  print_hex64(supervisor_start);
  uart0_puts("\n");

  uart0_puts("Kernel start address: ");
  print_hex64(kernel_start);
  uart0_puts("\n");

  handoff_supervisor = supervisor_start;
  handoff_kernel = kernel_start;
}
