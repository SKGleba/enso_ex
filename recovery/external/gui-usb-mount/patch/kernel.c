/* pre-recovery patcher
 *
 * Copyright (C) 2017 molecule, 2020-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../../plugins/hooking.h"
#include "../../../../plugins/plugins.h"

static const uint8_t sysstate_ret_patch[] = {0x13, 0x22, 0xc8, 0xf2, 0x01, 0x02};

static const char *custom_osz = "sdstor0:ext-lp-ign-entire";

// sigpatch globals
static int g_homebrew_decrypt = 0;
static int (*sbl_parse_header)(uint32_t ctx, const void *header, int len, void *args) = NULL;
static int (*sbl_set_up_buffer)(uint32_t ctx, int segidx) = NULL;
static int (*sbl_decrypt)(uint32_t ctx, void *buf, int sz) = NULL;

// sigpatches for bootup
static int sbl_parse_header_patched(uint32_t ctx, const void *header, int len, void *args) {
    int ret = sbl_parse_header(ctx, header, len, args);
    DACR_OFF(
        g_homebrew_decrypt = (ret < 0);
    );
    if (g_homebrew_decrypt) {
        *(uint32_t *)(args + 168) = 0x40; // hb
        ret = 0;
    }
    return ret;
}

static int sbl_set_up_buffer_patched(uint32_t ctx, int segidx) {
    if (g_homebrew_decrypt)
        return 2; // always compressed!
    return sbl_set_up_buffer(ctx, segidx);
}

static int sbl_decrypt_patched(uint32_t ctx, void *buf, int sz) {
    if (g_homebrew_decrypt)
        return 0;
    return sbl_decrypt(ctx, buf, sz);
}

// sdif globals
static int g_allow_bread = 0;
static int g_allow_bwrite = 0;
static void* (*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;

// block from reading mbr unless prev asked
static int sdif_read_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
    int ret;
    if (unlikely((uint32_t)ctx == 0xCAFEBABE)) {
        DACR_OFF(
            g_allow_bread = nSectors;
        );
        return 0xAA16;
    }
    if (sector == 0 && !g_allow_bread && nSectors > 0) {
        if (get_sd_context_part_validate_mmc(0) == ctx) {
            ret = sdif_read_sector_mmc(ctx, 1, buffer, 1);
            if (ret >= 0 && nSectors > 1) {
                ret = sdif_read_sector_mmc(ctx, 1, buffer + 0x200, nSectors - 1);
            }
            return ret;
        }
    }
    return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

// block from writing boot sectors unless prev asked
static int sdif_write_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
    if (unlikely((uint32_t)ctx == 0xCAFEBABE)) {
        DACR_OFF(
            g_allow_bwrite = nSectors;
        );
        return 0xAA16;
    }
    if ((sector < 0x200 || (sector < 0x8000 && sector > 0x4000)) && !g_allow_bwrite)
        return -1;
    return sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(uint32_t argc, void *args) {
	
	patch_args_struct *patch_args = args;
    if (patch_args->this_version != PATCH_ARGS_VERSION)
        return -1;
	
    SceObject *obj;
    SceModuleObject *mod;
	
	// config / stopfselfs patch
	obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_b[14]); // SysStateMgr
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
			INSTALL_RET_THUMB(mod->segments[0].buf + 0x1500, 1); // allow sd0:psp2config.skprx
            *(uint32_t*)(mod->segments[0].buf + 0xE28) = 0x20012001;
            patch_args->kbl_memcpy(mod->segments[0].buf + 0xD92, sysstate_ret_patch, sizeof(sysstate_ret_patch)); // skip iof ret err
		);
    }
	
	// fself/hen patch
    obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_a[9]); // AuthMgr
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		HOOK_EXPORT(sbl_parse_header, 0x7ABF5135, 0xF3411881);
		HOOK_EXPORT(sbl_set_up_buffer, 0x7ABF5135, 0x89CCDA2C);
		HOOK_EXPORT(sbl_decrypt, 0x7ABF5135, 0xBC422443);
    }
	
	// gc-sd compat patches
    obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_a[0]); // Sysmem
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
            INSTALL_RET_THUMB(mod->segments[0].buf + 0x20fc8, 1); // manufacturing mode
            *(uint16_t*)(mod->segments[0].buf + 0x21610) = 0x2001; // allow gc-sd mount
		);
    }
	
	// gc-sd patches - sdroot-as-os0 + sd0 patch
    obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_a[11]); // iofilemgr
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
			*(uint32_t *)(mod->segments[0].buf + 0x1d940) = (uint32_t)custom_osz;
		);
    }
    
    // sdif patches for R&W
    obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_b[7]); // sdif
    if (obj != NULL) {
        mod = (SceModuleObject*)&obj->data;
        HOOK_EXPORT(sdif_read_sector_mmc, 0x96D306FA, 0x6F8D529B);
        HOOK_EXPORT(sdif_write_sector_mmc, 0x96D306FA, 0x175543D2);
        DACR_OFF( // skip enso's patch
            sdif_read_sector_mmc = (void*)(mod->segments[0].buf + 0x3e7d);
            sdif_write_sector_mmc = (void*)(mod->segments[0].buf + 0x42d5);
        );
        FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
    }
    
    SceKernelAllocMemBlockKernelOpt optp;
    optp.size = 0x58;
    optp.attr = 2;
    optp.paddr = 0x1c000000;

    int mblk = patch_args->kbl_alloc_memblock("rcvrlogo", 0x6020D006, 0x200000, &optp);
    void* xbas = NULL;
    patch_args->kbl_get_memblock(mblk, (void**)&xbas);

    if (xbas == NULL)
        return 0;

    int logoerr = patch_args->ex_get_file("os0:ex/rcvr_logo.raw", xbas, 0x200000, 0);

    patch_args->kbl_free_memblock(mblk);
    
    if (logoerr >= 0) {
        obj = patch_args->kbl_get_obj_for_uid(patch_args->uids_b[4]);
        if (obj != NULL) {
            mod = (SceModuleObject*)&obj->data;
            DACR_OFF(
                *(uint32_t*)(mod->segments[0].buf + 0x700e) = 0x20002000;
            );
        }
    }
	
	return 0;
}

void _stop() __attribute__ ((weak, alias ("module_stop")));
int module_stop(void) {
	return 0;
}