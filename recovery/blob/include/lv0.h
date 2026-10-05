#ifndef __LV0_H__
#define __LV0_H__

#include "utils.h"
#include "fmgr.h"

struct lv0_ctx130_s {
    uint32_t unk_0;
    uint32_t self_type;  // 2 - user = 1 / kernel = 0
    char data0[0x90];    // hardcoded data
    char data1[0x90];
    uint32_t pathId;  // 2 (2 = os0)
    uint32_t unk_12C;
};

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

struct lv0_cmd_s {
    uint32_t size;
    uint32_t service_id;
    uint32_t response;
    uint32_t unk2;
    uint32_t padding[(0x40 - 0x10) / 4];
    uint32_t arg[];
};

#ifndef RXP_PIE
int lv0_load_sm(const char* path);
int lv0_stop_sm(void);
int lv0_call_sm(void* argv);
#else
#define r_lv0_load_sm(...) _r->lv0->lv0_load_sm(__VA_ARGS__)
#define r_lv0_stop_sm(...) _r->lv0->lv0_stop_sm(__VA_ARGS__)
#define r_lv0_call_sm(...) _r->lv0->lv0_call_sm(__VA_ARGS__)
#endif

struct exports_lv0_s {
    int (*lv0_load_sm)(const char* path);
    int (*lv0_stop_sm)(void);
    int (*lv0_call_sm)(void* argv);
};

#endif