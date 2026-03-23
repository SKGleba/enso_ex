#ifndef __STAGE2_H__
#define __STAGE2_H__

#include "paper.h"
#include "main.h"
#include "bootstrap.h"

enum STAGE2_GCSD_MODES {
	STAGE2_GCSD_MODE_DISABLED = 0,
	STAGE2_GCSD_MODE_SD0 = 1,
	STAGE2_GCSD_MODE_OS0 = 2,
	STAGE2_GCSD_MODE_INIT = 3
};

#define BOOTPATCH_TXTCFG "os0:/boot.patch"

#ifndef RXP_PIE
extern struct menu_s stage2_menu_s;
extern struct stage2_options stage2_opts;

int stage2_apply_config(void);
int stage2_menu(int selection);
#else
#define r_stage2_menu(...) _r->stage2->stage2_menu(__VA_ARGS__)
#define r_stage2_apply_config(...) _r->stage2->stage2_apply_config(__VA_ARGS__)
#define r_stage2_menu_s (_r->stage2->stage2_menu_s)
#define r_stage2_opts (_r->stage2->stage2_opts)
#endif

struct exports_stage2_s {
    int (*stage2_apply_config)(void);
    int (*stage2_menu)(int selection);
    struct menu_s *stage2_menu_s;
    struct stage2_options *stage2_opts;
};

#endif // __STAGE2_H__