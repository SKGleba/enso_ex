/* custom bootlogo patch
 *
 * Copyright (C) 2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../hooking.h"
#include "../patches.h"

#define DISPLAY_BOOT_LOGO_FUNC (0x6F9C)

static int is_safe_mode(kbl_param_struct *kblparam) {
    uint32_t v;
    if (kblparam->debug_flags[7] != 0xFF) {
        return 1;
    }
    v = kblparam->boot_type_indicator_2 & 0x7F;
    if (v == 0xB || (v == 4 && kblparam->resume_context_addr)) {
        v = ~kblparam->field_CC;
        if (((v >> 8) & 0x54) == 0x54 && (v & 0xC0) == 0) {
            return 1;
        } else {
            return 0;
        }
    } else if (v == 4) {
        return 0;
    }
    if (v == 0x1F || (uint32_t)(v - 0x18) <= 1) {
        return 1;
    } else {
        return 0;
    }
}

static int is_update_mode(kbl_param_struct *kblparam) {
    if (kblparam->debug_flags[4] != 0xFF) {
        return 1;
    } else {
        return 0;
    }
}

static inline int skip_patches(kbl_param_struct *kblparam) {
    return is_safe_mode(kblparam) || is_update_mode(kblparam);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(uint32_t argc, void *args) {
	
	patch_args_struct *patch_args = args;
	
	if (skip_patches(patch_args->kbl_param))
		return 0;
	
	int goodfw = (*(uint32_t *)(patch_args->kbl_param + 0x4) == 0x03650000);
	
	void *(*get_obj_for_uid)(int uid) = (goodfw) ? get_obj_for_uid_365 : get_obj_for_uid_360;
	int (*sceKernelAllocMemBlock)(const char *name, int type, int size, void *opt) = (goodfw) ? sceKernelAllocMemBlock_365 : sceKernelAllocMemBlock_360;
	int (*sceKernelGetMemBlockBase)(int32_t uid, void **basep) = (goodfw) ? sceKernelGetMemBlockBase_365 : sceKernelGetMemBlockBase_360;
	int (*sceKernelFreeMemBlock)(int32_t uid) = (goodfw) ? sceKernelFreeMemBlock_365 : sceKernelFreeMemBlock_360;
	int (*lf)(char *fpath, void *dst, unsigned int sz) = patch_args->load_file;
	
	SceKernelAllocMemBlockKernelOpt optp;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1c000000;
	
	int mblk = sceKernelAllocMemBlock("bootlogo", 0x6020D006, 0x200000, &optp);
	void *xbas = NULL;
	sceKernelGetMemBlockBase(mblk, (void **)&xbas);
	
	if (xbas == NULL)
		return 0;
	
	int logoerr = lf("os0:ex/bootlogo.raw", xbas, 0x200000);
	
	sceKernelFreeMemBlock(mblk);
	
	SceObject *obj;
    SceModuleObject *mod;
	
	obj = get_obj_for_uid(patch_args->uids_b[4]);
	if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
			if (logoerr == 0)
				*(uint32_t *)(mod->segments[0].buf + 0x700e) = 0x20002000;
			else
				INSTALL_RET_THUMB(mod->segments[0].buf + DISPLAY_BOOT_LOGO_FUNC, 0);
		);
	}
	
	return 0;
}

void _stop() __attribute__ ((weak, alias ("module_stop")));
int module_stop(void) {
	return 0;
}