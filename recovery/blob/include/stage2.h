#ifndef __STAGE2_H__
#define __STAGE2_H__

#include "paper.h"
#include "main.h"

enum STAGE2_GCSD_MODES {
	STAGE2_GCSD_MODE_DISABLED = 0,
	STAGE2_GCSD_MODE_SD0 = 1,
	STAGE2_GCSD_MODE_OS0 = 2,
	STAGE2_GCSD_MODE_INIT = 3
};

extern struct menu_s stage2_menu_s;
extern struct stage2_options stage2_opts;

int stage2_apply_config(void);
int stage2_menu(int selection);

#endif // __STAGE2_H__