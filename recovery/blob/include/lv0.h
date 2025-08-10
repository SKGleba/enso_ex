#ifndef __LV0_H__
#define __LV0_H__

#include "utils.h"
#include "fmgr.h"

#define LV0_DEFAULT_PAYLOAD_PA 0x1f850000
#define LV0_DEFAULT_PAYLOAD_SIZE 0x10000
#define LV0_SPL_BLOCK_PA LV0_DEFAULT_PAYLOAD_PA
#define LV0_SPL_PAYLOAD_PA (LV0_DEFAULT_PAYLOAD_PA + 0x40)

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
#else
#define r_lv0_initialized *(_r->lv0->lv0_initialized)
#define r_lv0_load_sm(...) _r->lv0->lv0_load_sm(__VA_ARGS__)
#define r_lv0_stop_sm(...) _r->lv0->lv0_stop_sm(__VA_ARGS__)
#define r_lv0_call_sm(...) _r->lv0->lv0_call_sm(__VA_ARGS__)
#define r_lv0_init(...) _r->lv0->lv0_init(__VA_ARGS__)
#define r_lv0_spl_exec(...) _r->lv0->lv0_spl_exec(__VA_ARGS__)
#endif

struct exports_lv0_s {
    int *lv0_initialized;
    int (*lv0_load_sm)(const char* path);
    int (*lv0_stop_sm)(void);
    int (*lv0_call_sm)(int svc, void* argv, uint32_t size);
    int (*lv0_init)(const char* ussm);
    int (*lv0_spl_exec)(void* payload, uint32_t paddr, int size, uint32_t arg);
};

#endif