#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../patches/hooking.h"
#include "../../../patches/patches.h"

typedef struct hfw_config_struct {
  uint32_t kasm_sz;
  uint32_t orig_fwv;
  uint32_t fw_fwv;
  uint32_t ussm_sz;
} __attribute__((packed)) hfw_config_struct;

static void (*sysmem_set_fwver_handler)(uint32_t handler) = NULL;
static int (*sysmem_get_fwver)(void) = NULL;
static int (*sysmem_get_sysroot_part2)(void) = NULL;

static int sysmem_get_fwver_patched(void) {
	return *(uint32_t *)(*(int *)(sysmem_get_sysroot_part2() + 0x6c) + 4);
}

static void sysmem_set_fwver_handler_patched(uint32_t handler) {
	sysmem_set_fwver_handler((uint32_t)sysmem_get_fwver_patched);
	return;
}

// WARNING: RUN IT AS THE LAST PATCH - IT CHANGES THE SYSROOT'S FWV
void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(uint32_t argc, void *args) {
	
	patch_args_struct *patch_args = args;
	
	int goodfw = (*(uint32_t *)(patch_args->kbl_param + 0x4) == 0x03650000);
	void *(*get_obj_for_uid)(int uid) = (goodfw) ? get_obj_for_uid_365 : get_obj_for_uid_360;
	
	int (*lf)(char *fpath, void *dst, unsigned int sz, int mode) = patch_args->load_file;
	hfw_config_struct cfg;
	if (lf("os0:hfw_cfg.bin", &cfg, 16, 2) != 0)
		return 0;
	if (cfg.fw_fwv > 0) {
		*(uint32_t *)(patch_args->kbl_param + 4) = cfg.fw_fwv; // kbl_param.fw_ver
		*(uint32_t *)((*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c)) + 4) = cfg.fw_fwv; // sysrootP2->kbl_param.fw_ver
	}
	
    SceObject *obj;
    SceModuleObject *mod;
	
    obj = get_obj_for_uid(patch_args->uids_a[0]);
    if (obj != NULL) {
		mod = (SceModuleObject *)&obj->data;
		HOOK_EXPORT(sysmem_set_fwver_handler, 0x2ED7F97A, 0x3276086B);
		HOOK_EXPORT(sysmem_get_fwver, 0x2ED7F97A, 0x67AAB627);
		FIND_EXPORT(sysmem_get_sysroot_part2, 0x3691DA45, 0x3E455842);
    }
	
	return 0;
}

void _stop() __attribute__ ((weak, alias ("module_stop")));
int module_stop(void) {
	return 0;
}