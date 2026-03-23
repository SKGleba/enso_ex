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

#define BSWAP16(x) ((((uint32_t)x << 8) & 0xff00) | (((uint32_t)x >> 8) & 0x00ff))
#define BSWAP24(x) ((((uint32_t)x << 16) & 0xff0000) | (((uint32_t)x >> 8) & 0x00ff00) | (((uint32_t)x >> 24) & 0x0000ff))
#define BSWAP32(x) ((((uint32_t)x << 24) & 0xff000000) | (((uint32_t)x << 8) & 0x00ff0000) | (((uint32_t)x >> 8) & 0x0000ff00) | (((uint32_t)x >> 24) & 0x000000ff))

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

enum LOG_LEVELS {
    LOGLEVEL_DEBUG = 0,
    LOGLEVEL_INFO,
    LOGLEVEL_WARN,
    LOGLEVEL_ERROR,
    LOGLEVEL_USER,
    LOGLEVEL_NONE
};

struct log_level_s {
    enum LOG_LEVELS console;
    enum LOG_LEVELS logpaper;
    enum LOG_LEVELS frontpage;
};

#define LOGLEVEL_CONSOLE   LOGLEVEL_DEBUG
#define LOGLEVEL_LOGPAPER  LOGLEVEL_INFO
#define LOGLEVEL_FRONTPAGE LOGLEVEL_USER

#define RMEMBLOCK_MIN_SIZE 0x1000
#define RMEMBLOCK_MAX_SIZE 0x1000000
#define RMEMBLOCK_BB_COUNT 4
#define RMEMBLOCK_SMB_SIZE 256
#define RMEMBLOCK_PA_COUNT 16  // first RMEMBLOCK_BB_COUNT reserved

struct rmemblock_info_s {
    int id;
    void *va;
};

struct rmemblock_master_s {
    struct rmemblock_info_s pallocs[RMEMBLOCK_PA_COUNT];
    uint16_t smbe[RMEMBLOCK_BB_COUNT];
};

#define ARMP_ARG_MAGIC 'ARMP'
struct armp_d_s {
    void *src;
    void *dst;
    uint32_t sz;
    union {
        int ret;
        bool src_fa;
    };
    struct armp_d_s *next;
};
struct armp_x_s {
    union {
        int (*func)(uint32_t arg);
        void *src;
    };
    uint32_t arg;
    union {
        uint32_t c_sz;
        int ret;
    };
    struct armp_x_s *next;
};
struct armp_arg_s {
    uint32_t magic;
    struct armp_d_s *d;
    struct armp_x_s *x;
};

enum CHAIN_FREE_TYPES {
    CHAIN_FREE_NONE = 0,
    CHAIN_FREE_NESTED = (1 << 0),
    CHAIN_FREE_ENTRIES = (1 << 1)
};

// --- GLOBALS ---
#ifndef RXP_PIE
extern int log_outputs;
void log(enum LOG_LEVELS level, const char *fmt, ...);
void log_hexdump(void *addr, int size, uint32_t show_addr, char delim);
extern struct log_level_s log_levels;
#define DLOG(fmt, ...) log(LOGLEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define ILOG(fmt, ...) log(LOGLEVEL_INFO, fmt, ##__VA_ARGS__)
#define WLOG(fmt, ...) log(LOGLEVEL_WARN, fmt, ##__VA_ARGS__)
#define ELOG(fmt, ...) log(LOGLEVEL_ERROR, fmt, ##__VA_ARGS__)
#define ULOG(fmt, ...) log(LOGLEVEL_USER, fmt, ##__VA_ARGS__)
#define lplog(fmt, ...) log(log_levels.logpaper, fmt, ##__VA_ARGS__)

#define BGW_START() gpio_port_set(0, GPIO_PORT_PS_LED)  // turn on the PS LED, indicates longer bg job
#define BGW_END() gpio_port_clear(0, GPIO_PORT_PS_LED)
#define lpreset()                                           \
    do {                                                    \
        paper_clear(&default_paper, default_paper.color);   \
        pen_reset(&default_paper, default_paper.pen.color); \
    } while (0)
#define lpclog(_color, fmt, ...)                       \
    do {                                                \
        uint32_t _prev_color = default_paper.pen.color; \
        default_paper.pen.color = _color;               \
        lplog(fmt, ##__VA_ARGS__);                     \
        default_paper.pen.color = _prev_color;          \
    } while (0)

#define EMMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define SDREAD(_sector, _buffer, _nsectors) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define EMMCWRITE(_sector, _buffer, _nsectors) write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define SDWRITE(_sector, _buffer, _nsectors) write_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)

#define _hexdump(addr, size) log_hexdump((void *)(addr), (size), 0xFFFFFFFF, ' ')
#define _hexdump_addr(addr, size, show_addr) log_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) log_hexdump((void *)(addr), (size), show_addr, delim)
#define hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)

void *rmemblock_alloc(int size, uint32_t opt_type, uint32_t opt_paddr);
int rmemblock_remap(void *va, uint32_t type);
#define rmemblock_free(_va) rmemblock_remap((_va), 0)
extern struct rmemblock_master_s rmemblock_master;
#define rmemblock_start() memset(&rmemblock_master, -1, sizeof(rmemblock_master));
void rmemblock_stop(void);

char *my_strchr(const char *s, char c);
char *my_strnchr(const char *s, char c, int len);
char *my_strrchr(const char *s, char c);
int count_chs(const char *s, char c);
char *find_nth(const char *s, char c, int n);
char *find_rnth(const char *s, char c, int n);
int antoh(char *input, uint8_t *output, int output_len);
int hntoa(uint8_t *input, char *output, int output_len);

int idstorage_init(void);
int idstorage_stop(void);
int idstorage_rw_leaf(bool write, uint16_t leaf, void *buf);

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

int armp_run(struct armp_arg_s *armp, enum CHAIN_FREE_TYPES free);

#define my_malloc(_size) rmemblock_alloc((_size), 0, 0)
#define my_free(_va) rmemblock_free((_va))
#define my_rxmap(_va) rmemblock_remap((_va), MEMBLOCK_TYPE_RX)

#define my_snprintf(_buf, _size, _fmt, ...) nskbl_snprintf((_buf), (_size), (_fmt), ##__VA_ARGS__)
#define my_strncmp(_s1, _s2, _len) nskbl_strncmp((_s1), (_s2), (_len))

#define idstorage_sync idstorage_init

#else
#define r_log_outputs *(_r->utils->log_outputs)
#define r_log_levels (_r->utils->log_levels)
#define r_log(...) _r->utils->log(__VA_ARGS__)
#define r_DLOG(...) r_log(LOGLEVEL_DEBUG, __VA_ARGS__)
#define r_ILOG(...) r_log(LOGLEVEL_INFO, __VA_ARGS__)
#define r_ELOG(...) r_log(LOGLEVEL_ERROR, __VA_ARGS__)
#define r_WLOG(...) r_log(LOGLEVEL_WARNING, __VA_ARGS__)
#define r_ULOG(...) r_log(LOGLEVEL_USER, __VA_ARGS__)
#define r_lplog(...) r_log(r_log_levels.logpaper, __VA_ARGS__)
#define r_BGW_START() _r->lbm->gpio_port_set(0, GPIO_PORT_PS_LED)  // turn on the PS LED, indicates longer bg job
#define r_BGW_END() _r->lbm->gpio_port_clear(0, GPIO_PORT_PS_LED)
#define r_EMMCREAD(_sector, _buffer, _nsectors) read_sector_mmc_direct((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define r_SDREAD(_sector, _buffer, _nsectors) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define r_EMMCWRITE(_sector, _buffer, _nsectors) r_write_sector_mmc((int *)*(uint32_t *)NSKBL_DEVICE_EMMC_TGT_CTX, _sector, _buffer, _nsectors)
#define r_SDWRITE(_sector, _buffer, _nsectors) r_write_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, _sector, _buffer, _nsectors)
#define _hexdump(addr, size) r_dbg_hexdump((void *)(addr), (size), 0xFFFFFFFF, ' ')
#define _hexdump_addr(addr, size, show_addr) r_dbg_hexdump((void *)(addr), (size), show_addr, ' ')
#define _hexdump_full(addr, size, show_addr, delim) r_dbg_hexdump((void *)(addr), (size), show_addr, delim)
#define r_hexdump(...) FUN_VAR4(__VA_ARGS__, _hexdump_full, _hexdump_addr, _hexdump)(__VA_ARGS__)
#define r_memblock_master _r->utils->memblock_master
#define r_rmemblock_alloc(...) _r->utils->rmemblock_alloc(__VA_ARGS__)
#define r_memblock_free(_va) _r->utils->rmemblock_remap((_va), 0)
#define r_memblock_remap(...) _r->utils->rmemblock_remap(__VA_ARGS__)
#define r_memblock_stop() _r->utils->rmemblock_stop()
#define r_memblock_start() _r->lbm->memset(_r->utils->memblock_master, -1, sizeof(struct rmemblock_master_s))
#define r_strchr(...) _r->utils->my_strchr(__VA_ARGS__)
#define r_strnchr(...) _r->utils->my_strnchr(__VA_ARGS__)
#define r_strrchr(...) _r->utils->my_strrchr(__VA_ARGS__)
#define r_count_chs(...) _r->utils->count_chs(__VA_ARGS__)
#define r_find_nth(...) _r->utils->find_nth(__VA_ARGS__)
#define r_find_rnth(...) _r->utils->find_rnth(__VA_ARGS__)
#define r_antoh(...) _r->lbm->antoh(__VA_ARGS__)
#define r_hntoa(...) _r->lbm->hntoa(__VA_ARGS__)
#define r_find_endline(...) _r->utils->find_endline(__VA_ARGS__)
#define r_find_nextline(...) _r->utils->find_nextline(__VA_ARGS__)
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
#define r_armp_run(...) _r->utils->armp_run(__VA_ARGS__)

#endif

struct exports_util_s {
    int *log_outputs;
    struct log_level_s *log_levels;
    void (*log)(enum LOG_LEVELS level, const char *fmt, ...);
    void (*log_hexdump)(void *addr, int size, uint32_t show_addr, char delim);
    struct rmemblock_master_s *memblock_master;
    void *(*rmemblock_alloc)(int size, uint32_t opt_type, uint32_t opt_paddr);
    int (*rmemblock_remap)(void *va, uint32_t type);
    void (*rmemblock_stop)(void);
    char *(*my_strchr)(const char *s, char c);
    char *(*my_strnchr)(const char *s, char c, int len);
    char *(*my_strrchr)(const char *s, char c);
    int (*count_chs)(const char *s, char c);
    char *(*find_nth)(const char *s, char c, int n);
    char *(*find_rnth)(const char *s, char c, int n);
    int (*antoh)(char *input, uint8_t *output, int output_len);
    int (*hntoa)(uint8_t *input, char *output, int output_len);
    int (*idstorage_init)(void);
    int (*idstorage_stop)(void);
    int (*idstorage_rw_leaf)(bool write, uint16_t leaf, void *buf);
    uint32_t (*crc32)(uint32_t crc, const void *buf, size_t size);
    int (*armp_run)(struct armp_arg_s *armp, enum CHAIN_FREE_TYPES free);
};

#endif