#include <inttypes.h>

#ifdef FW_360
	#include "../enso/360/nsbl.h"
#else
	#include "../enso/365/nsbl.h"
#endif

#include "../enso/bm_compat.h"
#include "../enso/ex_defs.h"
	
#define DACR_OFF(stmt)                 \
do {                                   \
    unsigned prev_dacr;                \
    __asm__ volatile(                  \
        "mrc p15, 0, %0, c3, c0, 0 \n" \
        : "=r" (prev_dacr)             \
    );                                 \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (0xFFFF0000)           \
    );                                 \
    stmt;                              \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (prev_dacr)            \
    );                                 \
} while (0)

static const char ux0_path[] = "ux0:";
static const char ux0_psp2config_path[] = "ux0:eex/boot_config.txt";
	
void _start(PayloadArgsStruct *args) {
	if (args->trun != 2)
		return;
	SceObject *obj;
    SceModuleObject *mod;
	const SceModuleLoadList *list = args->list;
	for (int i = 0; i < args->count; i-=-1) {
		if (!list[i].filename)
            continue;
        if (CTRL_BUTTON_HELD(args->ctrldata, E2X_EPATCHES_UXCFG) && strncmp(list[i].filename, "sysstatemgr.skprx", 17) == 0) {
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				DACR_OFF(
					memcpy(mod->segments[0].buf + SYSSTATE_SD0_STRING, ux0_path, sizeof(ux0_path));
					memcpy(mod->segments[0].buf + SYSSTATE_SD0_PSP2CONFIG_STRING, ux0_psp2config_path, sizeof(ux0_psp2config_path));
				);
			}
		}
    }
	return;
}