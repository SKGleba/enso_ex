#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../patches/hooking.h"
#include "../../../patches/patches.h"

#define SDSTOR_PROC_CHKO (0x2498)
#define SDSTOR_PROC_CHKT (0x2940)
static const char sdstor_suppl_patch[4] = {0x01, 0x20, 0x00, 0xBF};
static const char movsr01[2] = {0x01, 0x20};

// sdif funcs
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_read_sector_sd)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_sd)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void *(*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;
static void *(*get_sd_context_part_validate_sd)(int sd_ctx_index) = NULL;

static int sdif_read_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (get_sd_context_part_validate_mmc(0) == ctx) {
		int ret = sdif_read_sector_sd(get_sd_context_part_validate_sd(1), sector, buffer, nSectors);
		if (ret < 0)
			ret = sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
		return ret;
	}
	return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

static int sdif_write_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (get_sd_context_part_validate_mmc(0) == ctx) {
		int ret = sdif_write_sector_sd(get_sd_context_part_validate_sd(1), sector, buffer, nSectors);
		if (ret < 0)
			ret = sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
		return ret;
	}
	return sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(uint32_t argc, void *args) {
	
	patch_args_struct *patch_args = args;
	
	if (!CTRL_BUTTON_HELD(patch_args->ctrldata, E2X_RECOVERY_RUNDN))
		return 0;
	
    SceObject *obj;
    SceModuleObject *mod;
	
	obj = get_obj_for_uid(patch_args->uids_a[0]);
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
			memcpy(mod->segments[0].buf + 0x21610, movsr01, sizeof(movsr01));
		);
    }
	
    obj = get_obj_for_uid(patch_args->uids_b[7]);
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		HOOK_EXPORT(sdif_read_sector_mmc, 0x96D306FA, 0x6F8D529B);
		FIND_EXPORT(sdif_read_sector_sd, 0x96D306FA, 0xB9593652);
		FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
		FIND_EXPORT(get_sd_context_part_validate_sd, 0x96D306FA, 0xB9EA5B1E);
		FIND_EXPORT(sdif_write_sector_sd, 0x96D306FA, 0xE0781171);
		HOOK_EXPORT(sdif_write_sector_mmc, 0x96D306FA, 0x175543D2);
    }
	
	obj = get_obj_for_uid(patch_args->uids_b[10]);
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		DACR_OFF(
			memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKO, sdstor_suppl_patch, sizeof(sdstor_suppl_patch));
			memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKT, sdstor_suppl_patch, sizeof(sdstor_suppl_patch));
		);
    }
	
	return 0;
}

void _stop() __attribute__ ((weak, alias ("module_stop")));
int module_stop(void) {
	return 0;
}