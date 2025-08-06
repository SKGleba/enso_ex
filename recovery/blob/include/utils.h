#ifndef __UTILS_H__
#define __UTILS_H__

#include <baremetal/gpio.h>

#include "nskbl.h"
#include "paper.h"
#include "stor.h"

#define LOG(fmt, ...) dbg_log(g_log_targets, fmt, ##__VA_ARGS__)
#define conlog(fmt, ...) dbg_log(LOG_TARGET_CONSOLE, fmt, ##__VA_ARGS__)
#define alllog(fmt, ...) dbg_log(-1, fmt, ##__VA_ARGS__)
#define inflog(fmt, ...) dbg_log(LOG_TARGET_FRONTPAGE, fmt, ##__VA_ARGS__)
#define scrlog(fmt, ...) dbg_log(LOG_TARGET_LOGPAPER, fmt, ##__VA_ARGS__)

#define BGW_START() gpio_port_set(0, GPIO_PORT_PS_LED) // turn on the PS LED, indicates longer bg job
#define BGW_END() gpio_port_clear(0, GPIO_PORT_PS_LED)

#define IHBGW(stmt) \
	do { \
		BGW_START(); \
		stmt; \
		BGW_END(); \
	} while (0)

#define EMMCWRITE(_sector, _buffer, _nsectors) write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define EMMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
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

#define DACR_OFF(stmt)                                                        \
    do {                                                                      \
        unsigned prev_dacr;                                                   \
        __asm__ volatile("mrc p15, 0, %0, c3, c0, 0 \n" : "=r"(prev_dacr));   \
        __asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(0xFFFF0000)); \
        stmt;                                                                 \
        __asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(prev_dacr));  \
    } while (0)

enum LOG_TARGETS {
	LOG_TARGET_NONE = 0,
	LOG_TARGET_CONSOLE = 1 << 0,
	LOG_TARGET_LOGPAPER = 1 << 1,
	LOG_TARGET_FRONTPAGE = 1 << 2,
};
extern int g_log_targets;
void dbg_log(int targets, const char *fmt, ...);
void dbg_hexdump(void *addr, int size, bool show_addr, char delim);

#define _hexdump(addr, size) dbg_hexdump((void *)(addr), (size), false, ' ')
#define _hexdump_addr(addr, size, show_addr) dbg_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) dbg_hexdump((void *)(addr), (size), show_addr, delim)
#define hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)

#define RMEMBLOCK_MIN_SIZE 0x1000
#define RMEMBLOCK_MAX_COUNT 16
struct rmemblock_s {
    int id;
    void *va;
};
extern struct rmemblock_s rmemblock_allocs[RMEMBLOCK_MAX_COUNT];
#define rmemblock_init() \
	do { \
		for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) { \
			rmemblock_allocs[i].id = -1; \
			rmemblock_allocs[i].va = NULL; \
		} \
	} while (0)
void *rmemblock_alloc(int size, uint32_t opt_type, uint32_t opt_paddr);
int rmemblock_free(void *va);
int rmemblock_remap(void *va, uint32_t type);
#define my_malloc(_size) rmemblock_alloc((_size), 0, 0)
#define my_free(_va) rmemblock_free((_va))
#define my_rxmap(_va) rmemblock_remap((_va), MEMBLOCK_TYPE_RX)

char *my_strchr(const char *s, int c);
char *my_strrchr(const char *s, int c);
int count_chs(const char *s, char c);
char *find_nth(const char *s, char c, int n);
char *find_rnth(const char *s, char c, int n);
#define my_snprintf(_buf, _size, _fmt, ...) nskbl_snprintf((_buf), (_size), (_fmt), ##__VA_ARGS__)
#define my_strncmp(_s1, _s2, _len) nskbl_strncmp((_s1), (_s2), (_len))

#endif