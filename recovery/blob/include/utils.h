#ifndef __UTILS_H__
#define __UTILS_H__

#include "nskbl.h"
#include "paper.h"
#include "stor.h"

#define LOG(fmt, ...) dbg_log(g_log_targets, fmt, ##__VA_ARGS__)
#define conlog(fmt, ...) dbg_log(LOG_TARGET_CONSOLE, fmt, ##__VA_ARGS__)
#define scrlog(fmt, ...) dbg_log(LOG_TARGET_PAPER, fmt, ##__VA_ARGS__)

#define MMCWRITE(_sector, _buffer, _nsectors) write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define MMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define SDREAD(_sector, _buffer, _nsectors) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define SDWRITE(_sector, _buffer, _nsectors) write_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)

#define ARRAYSIZE(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))
#define CONCAT44(high, low) ((((uint64_t)high) << 32) | ((uint64_t)low))

// atrocious, but i love it
#define vp *(volatile uint32_t *)
#define v8p *(volatile uint8_t *)
#define v16p *(volatile uint16_t *)
#define v32p *(volatile uint32_t *)
#define v64p *(volatile uint64_t *)

#define BITF(n) (~(-1 << (n)))
#define BITFL(n) (BITF((n) + 1))
#define BITN(n) (1 << (n))
#define BITNVAL(n, val) ((val) << (n))
#define BITNVALM(n, val, mask) (((val) & (mask)) << (n))
#define XBITN(v, n) (((v) >> (n)) & 1)
#define XBITNVALM(v, n, mask) (((v) >> (n)) & (mask))

// function selector based on argc
#define FUN_VAR4(_1, _2, _3, _4, _fun, ...) _fun

enum LOG_TARGETS {
	LOG_TARGET_NONE = 0,
	LOG_TARGET_CONSOLE = 1 << 0,
	LOG_TARGET_PAPER = 1 << 1,
};

extern int g_log_targets;

void dbg_log(int targets, const char *fmt, ...);
void dbg_hexdump(void *addr, int size, bool show_addr, char delim);

#define _hexdump(addr, size) dbg_hexdump((void *)(addr), (size), false, ' ')
#define _hexdump_addr(addr, size, show_addr) dbg_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) dbg_hexdump((void *)(addr), (size), show_addr, delim)
#define hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)

#endif