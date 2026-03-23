#ifndef __LV0_H__
#define __LV0_H__

#include "utils.h"
#include "fmgr.h"
#include "lv0p.h"

#define LV0_SPL_BLOCK_PA 0x1f850000
#define LV0_SPL_BLOCK_SIZE 0x10000
#define LV0_SPL_FMNFO_SZ 0x20
#define LV0_SPL_PAYLOAD_PA (LV0_SPL_BLOCK_PA + LV0_SPL_FMNFO_SZ)
#define LV0_SPL_PATCHER_SIZE 0x300
#define LV0_SPL_INIT_WORKBUF_OFF 0x40
#define LV0_SPL_INIT_PATCHER_OFF 0x100
#define LV0_SPL_INIT_XENTRY_OFF (LV0_SPL_INIT_PATCHER_OFF + LV0_SPL_PATCHER_SIZE)
#define LV0_SPL_INIT_FCMD_HANDLER_PA 0x00809e00
#define LV0_SPL_INIT_FCMD_HANDLER_SZ 0x100
#define LV0_SPL_INIT_U32_PATCH_ADDR 0x00800372
#define LV0_SPL_INIT_U32_PATCH_DATA 0x009adc79
#define LV0_SPL_INIT_U16_PATCH_ADDR 0x00800afe
#define LV0_SPL_INIT_U16_PATCH_DATA 0x0610
#define LV0_SPL_LV0P_BIG_ADDR 0x1c000000
#define LV0_SPL_LV0P_BIG_SIZE 0x00200000
#define LV0_SPL_LV0P_BIG_MAXESIZE 0x00100000

typedef struct lv0_spl_fm_nfo_s {
    uint16_t magic;
    uint8_t unused;
    uint8_t status;
    uint32_t codepaddr;
    uint32_t arg;
    uint32_t resp;
} __attribute__((packed)) lv0_spl_fm_nfo;

struct lv0_pa_pair_s {
    uint32_t addr;
    uint32_t length;
};

struct lv0_paddr_list_s {
    uint32_t size;
    uint32_t list_size;
    uint32_t ret_length;
    uint32_t ret_count;
    struct lv0_pa_pair_s* list;
};

struct lv0_heap_hdr_s {
    void* data;
    uint32_t size;
    uint32_t size_aligned;
    uint32_t padding;
    struct lv0_heap_hdr_s* prev;
    struct lv0_heap_hdr_s* next;
};

struct lv0_ctx130_s {
    uint32_t unk_0;
    uint32_t self_type;  // 2 - user = 1 / kernel = 0
    char data0[0x90];    // hardcoded data
    char data1[0x90];
    uint32_t pathId;  // 2 (2 = os0)
    uint32_t unk_12C;
};

struct lv0_cmd_0x50002_s {
    uint32_t unused_0[2];
    uint32_t use_lv2_mode_0;  // if 1, use lv2 list
    uint32_t use_lv2_mode_1;  // if 1, use lv2 list
    uint32_t unused_10[3];
    uint32_t list_count;  // must be < 0x1F1
    uint32_t unused_20[4];
    uint32_t total_count;  // only used in LV1 mode
    uint32_t unused_34[1];
    union {
        struct lv0_pa_pair_s lv1[0x1F1];
        struct lv0_pa_pair_s lv2[0x1F1];
    } list;
};

struct lv0_corrupt_args_cmd_s {
    unsigned int size;
    unsigned int service_id;
    unsigned int response;
    unsigned int unk2;
    unsigned int padding[(0x40 - 0x10) / 4];
    struct lv0_cmd_0x50002_s cargs;
};

struct lv0_jump_args_cmd_s {
    unsigned int size;
    unsigned int service_id;
    unsigned int response;
    unsigned int unk2;
    unsigned int padding[(0x40 - 0x10) / 4];
    uint32_t req[16];
};

#ifndef RXP_PIE
extern int lv0_initialized;
int lv0_load_sm(const char* path);
int lv0_stop_sm(void);
int lv0_call_sm(int svc, void* argv, uint32_t size);
int lv0_init(const char* ussm);
int lv0_spl_exec(void* payload, uint32_t paddr, int size, uint32_t arg);
extern unsigned char lv0p_nmp[];
extern unsigned int lv0p_nmp_len;
int lv0p_run(struct lv0p_arg_s* argv, uint32_t argp, uint32_t args, enum CHAIN_FREE_TYPES free);
#else
#define r_lv0_initialized *(_r->lv0->lv0_initialized)
#define r_lv0_load_sm(...) _r->lv0->lv0_load_sm(__VA_ARGS__)
#define r_lv0_stop_sm(...) _r->lv0->lv0_stop_sm(__VA_ARGS__)
#define r_lv0_call_sm(...) _r->lv0->lv0_call_sm(__VA_ARGS__)
#define r_lv0_init(...) _r->lv0->lv0_init(__VA_ARGS__)
#define r_lv0_spl_exec(...) _r->lv0->lv0_spl_exec(__VA_ARGS__)
#define r_lv0p_nmp _r->lv0->lv0p_nmp
#define r_lv0p_nmp_len *(_r->lv0->lv0p_nmp_len)
#define r_lv0p_run(...) _r->lv0->lv0p_run(__VA_ARGS__)
#endif

struct exports_lv0_s {
    int *lv0_initialized;
    int (*lv0_load_sm)(const char* path);
    int (*lv0_stop_sm)(void);
    int (*lv0_call_sm)(int svc, void* argv, uint32_t size);
    int (*lv0_init)(const char* ussm);
    int (*lv0_spl_exec)(void* payload, uint32_t paddr, int size, uint32_t arg);
    unsigned char **lv0p_nmp;
    unsigned int *lv0p_nmp_len;
    int (*lv0p_run)(struct lv0p_arg_s* argv, uint32_t argp, uint32_t args, enum CHAIN_FREE_TYPES free);
};

#endif