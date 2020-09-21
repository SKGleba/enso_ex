#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../patches/hooking.h"
#include "../../../patches/patches.h"

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

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(uint32_t argc, void *args) {
	
	patch_args_struct *patch_args = args;
	
	int goodfw = (*(uint32_t *)(patch_args->kbl_param + 0x4) == 0x03650000);
	
	void *(*get_obj_for_uid)(int uid) = (goodfw) ? get_obj_for_uid_365 : get_obj_for_uid_360;
	
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