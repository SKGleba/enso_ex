#ifndef __UTILS_H__
#define __UTILS_H__

#include <baremetal/gpio.h>

#include "nskbl.h"
#include "paper.h"
#include "stor.h"

#define IHBGW(stmt) \
	do { \
		BGW_START(); \
		stmt; \
		BGW_END(); \
	} while (0)

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

#define RMEMBLOCK_MIN_SIZE 0x1000
#define RMEMBLOCK_MAX_COUNT 16
struct rmemblock_s {
    int id;
    void *va;
};

// --- GLOBALS ---
#ifndef RXP_PIE
extern int g_log_targets;
void dbg_log(int targets, const char *fmt, ...);
void dbg_hexdump(void *addr, int size, bool show_addr, char delim);
#define LOG(fmt, ...) dbg_log(g_log_targets, fmt, ##__VA_ARGS__)
#define conlog(fmt, ...) dbg_log(LOG_TARGET_CONSOLE, fmt, ##__VA_ARGS__)
#define alllog(fmt, ...) dbg_log(-1, fmt, ##__VA_ARGS__)
#define inflog(fmt, ...) dbg_log(LOG_TARGET_FRONTPAGE, fmt, ##__VA_ARGS__)
#define scrlog(fmt, ...) dbg_log(LOG_TARGET_LOGPAPER, fmt, ##__VA_ARGS__)

#define BGW_START() gpio_port_set(0, GPIO_PORT_PS_LED)  // turn on the PS LED, indicates longer bg job
#define BGW_END() gpio_port_clear(0, GPIO_PORT_PS_LED)
#define screset()                                           \
    do {                                                    \
        paper_clear(&default_paper, default_paper.color);   \
        pen_reset(&default_paper, default_paper.pen.color); \
    } while (0)
#define scrclog(_color, fmt, ...)                       \
    do {                                                \
        uint32_t _prev_color = default_paper.pen.color; \
        default_paper.pen.color = _color;               \
        scrlog(fmt, ##__VA_ARGS__);                     \
        default_paper.pen.color = _prev_color;          \
    } while (0)

#define EMMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define SDREAD(_sector, _buffer, _nsectors) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define EMMCWRITE(_sector, _buffer, _nsectors) write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define SDWRITE(_sector, _buffer, _nsectors) write_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)

#define _hexdump(addr, size) dbg_hexdump((void *)(addr), (size), false, ' ')
#define _hexdump_addr(addr, size, show_addr) dbg_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) dbg_hexdump((void *)(addr), (size), show_addr, delim)
#define hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)

extern struct rmemblock_s rmemblock_allocs[RMEMBLOCK_MAX_COUNT];
void *rmemblock_alloc(int size, uint32_t opt_type, uint32_t opt_paddr);
int rmemblock_free(void *va);
int rmemblock_remap(void *va, uint32_t type);
#define rmemblock_init()                                \
    do {                                                \
        for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) { \
            rmemblock_allocs[i].id = -1;                \
            rmemblock_allocs[i].va = NULL;              \
        }                                               \
    } while (0)

char *my_strchr(const char *s, int c);
char *my_strrchr(const char *s, int c);
int count_chs(const char *s, char c);
char *find_nth(const char *s, char c, int n);
char *find_rnth(const char *s, char c, int n);

int idstorage_init(void);
int idstorage_stop(void);
int idstorage_rw_leaf(bool write, uint16_t leaf, void *buf);

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

#define my_malloc(_size) rmemblock_alloc((_size), 0, 0)
#define my_free(_va) rmemblock_free((_va))
#define my_rxmap(_va) rmemblock_remap((_va), MEMBLOCK_TYPE_RX)

#define my_snprintf(_buf, _size, _fmt, ...) nskbl_snprintf((_buf), (_size), (_fmt), ##__VA_ARGS__)
#define my_strncmp(_s1, _s2, _len) nskbl_strncmp((_s1), (_s2), (_len))

#define idstorage_sync idstorage_init

#else
#define r_g_log_targets *(_r->utils->g_log_targets)
#define r_dbg_log(...) _r->utils->dbg_log(__VA_ARGS__)
#define r_dbg_hexdump(...) _r->utils->dbg_hexdump(__VA_ARGS__)
#define r_LOG(fmt, ...) r_dbg_log(r_g_log_targets, fmt, ##__VA_ARGS__)
#define r_conlog(fmt, ...) r_dbg_log(LOG_TARGET_CONSOLE, fmt, ##__VA_ARGS__)
#define r_alllog(fmt, ...) r_dbg_log(-1, fmt, ##__VA_ARGS__)
#define r_inflog(fmt, ...) r_dbg_log(LOG_TARGET_FRONTPAGE, fmt, ##__VA_ARGS__)
#define r_scrlog(fmt, ...) r_dbg_log(LOG_TARGET_LOGPAPER, fmt, ##__VA_ARGS__)
#define r_screset()                                                                 \
    do {                                                                          \
        paper_clear(default_paper, default_paper.color);    \
        pen_reset(default_paper, default_paper.pen.color); \
    } while (0)
#define r_scrclog(_color, fmt, ...)                                  \
    do {                                                           \
        uint32_t _prev_color = default_paper.pen.color; \
        default_paper.pen.color = _color;               \
        r_scrlog(fmt, ##__VA_ARGS__);                                \
        default_paper.pen.color = _prev_color;          \
    } while (0)
#define r_BGW_START() _r->lbm->gpio_port_set(0, GPIO_PORT_PS_LED)  // turn on the PS LED, indicates longer bg job
#define r_BGW_END() _r->lbm->gpio_port_clear(0, GPIO_PORT_PS_LED)
#define r_EMMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define r_SDREAD(_sector, _buffer, _nsectors) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define r_EMMCWRITE(_sector, _buffer, _nsectors) r_write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define r_SDWRITE(_sector, _buffer, _nsectors) r_write_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define _hexdump(addr, size) r_dbg_hexdump((void *)(addr), (size), false, ' ')
#define _hexdump_addr(addr, size, show_addr) r_dbg_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) r_dbg_hexdump((void *)(addr), (size), show_addr, delim)
#define r_hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)
#define r_rmemblock_allocs (_r->utils->rmemblock_allocs)
#define r_rmemblock_alloc(...) _r->utils->rmemblock_alloc(__VA_ARGS__)
#define r_memblock_free(...) _r->utils->rmemblock_free(__VA_ARGS__)
#define r_memblock_remap(...) _r->utils->rmemblock_remap(__VA_ARGS__)
#define r_rmemblock_init()                                \
    do {                                                \
        for (int i = 0; i < RMEMBLOCK_MAX_COUNT; i++) { \
            r_rmemblock_allocs[i].id = -1;                \
            r_rmemblock_allocs[i].va = NULL;              \
        }                                               \
    } while (0)
#define r_strchr(...) _r->utils->my_strchr(__VA_ARGS__)
#define r_strrchr(...) _r->utils->my_strrchr(__VA_ARGS__)
#define r_count_chs(...) _r->utils->count_chs(__VA_ARGS__)
#define r_find_nth(...) _r->utils->find_nth(__VA_ARGS__)
#define r_find_rnth(...) _r->utils->find_rnth(__VA_ARGS__)
#define r_idstorage_init(...) _r->utils->idstorage_init(__VA_ARGS__)
#define r_idstorage_stop(...) _r->utils->idstorage_stop(__VA_ARGS__)
#define r_idstorage_rw_leaf(...) _r->utils->idstorage_rw_leaf(__VA_ARGS__)
#define r_crc32(...) _r->utils->crc32(__VA_ARGS__)
#define r_malloc(_size) r_rmemblock_alloc((_size), 0, 0)
#define r_free(_va) r_memblock_free((_va))
#define r_rxmap(_va) r_memblock_remap((_va), MEMBLOCK_TYPE_RX)
#define r_snprintf(_buf, _size, _fmt, ...) nskbl_snprintf((_buf), (_size), (_fmt), ##__VA_ARGS__)
#define r_strncmp(_s1, _s2, _len) nskbl_strncmp((_s1), (_s2), (_len))
#define r_idstorage_sync r_idstorage_init
#endif

struct exports_util_s {
    int *g_log_targets;
    void (*dbg_log)(int targets, const char *fmt, ...);
    void (*dbg_hexdump)(void *addr, int size, bool show_addr, char delim);
    struct rmemblock_s *rmemblock_allocs;
    void *(*rmemblock_alloc)(int size, uint32_t opt_type, uint32_t opt_paddr);
    int (*rmemblock_free)(void *va);
    int (*rmemblock_remap)(void *va, uint32_t type);
    char *(*my_strchr)(const char *s, int c);
    char *(*my_strrchr)(const char *s, int c);
    int (*count_chs)(const char *s, char c);
    char *(*find_nth)(const char *s, char c, int n);
    char *(*find_rnth)(const char *s, char c, int n);
    int (*idstorage_init)(void);
    int (*idstorage_stop)(void);
    int (*idstorage_rw_leaf)(bool write, uint16_t leaf, void *buf);
    uint32_t (*crc32)(uint32_t crc, const void *buf, size_t size);
};

#endif