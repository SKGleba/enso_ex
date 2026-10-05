#ifndef __SPL_H__
#define __SPL_H__

#include "utils.h"

#define LSPL_CFGSEL_LV0EN
#define LSPL_CFGSEL_LV0P_FULL
#define LSPL_CFGSEL_TZS_EN
#include <libspl/libspl.h>

#ifndef RXP_PIE
extern int spl_NOTinitialized;
int spl_init(const char *ussm_path, int flags);
int spl_deinit(int flags);
int spl_lv0p_run(struct splv0p_arg_s *argv, enum CHAIN_FREE_TYPES free);
int spl_lv0_exec(void *payload, int size, uint32_t pa_pa, uint32_t stack, int a0, int a1, int a2, int a3);
#else
#define r_spl_NOTinitialized *(_r->spl->spl_NOTinitialized)
#define r_spl_init(...) _r->spl->spl_init(__VA_ARGS__)
#define r_spl_deinit(...) _r->spl->spl_deinit(__VA_ARGS__)
#define r_spl_lv0p_run(...) _r->spl->spl_lv0p_run(__VA_ARGS__)
#define r_spl_lv0_exec(...) _r->spl->spl_lv0_exec(__VA_ARGS__)
#endif // RXP_PIE

struct exports_spl_s {
    int *spl_NOTinitialized;
    int (*spl_init)(const char *ussm_path, int flags);
    int (*spl_deinit)(int flags);
    int (*spl_lv0p_run)(struct splv0p_arg_s *argv, enum CHAIN_FREE_TYPES free);
    int (*spl_lv0_exec)(void *payload, int size, uint32_t pa_pa, uint32_t stack, int a0, int a1, int a2, int a3);
};

#endif // __SPL_H__