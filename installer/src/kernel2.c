/* kernel2.c -- modulemgr workaround
 *
 * Copyright (C) 2017 molecule, 2018-2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2kern/kernel/modulemgr.h>
#include <stdio.h>
#include <string.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/threadmgr.h>

#include <taihen.h>

static tai_hook_ref_t unload_allowed_hook;
static SceUID unload_allowed_uid;

int unload_allowed_patched(void) {
	TAI_CONTINUE(int, unload_allowed_hook);
	return 1; // always allowed
}

int module_start(int args, void *argv) {
	(void)args;
	(void)argv;
	unload_allowed_uid = taiHookFunctionImportForKernel(KERNEL_PID, 
		&unload_allowed_hook,     // Output a reference
		"SceKernelModulemgr",     // Name of module being hooked
		0x11F9B314,               // NID specifying SceSblACMgrForKernel
		0xBBA13D9C,               // Function NID
		unload_allowed_patched);  // Name of the hook function
	
	ksceIoUmount(0x200, 0, 0, 0);
	ksceIoUmount(0x200, 1, 0, 0);
	ksceIoMount(0x200, NULL, 2, 0, 0, 0);

	return SCE_KERNEL_START_SUCCESS;
}
void _start() __attribute__ ((weak, alias ("module_start")));

int module_stop() {
	taiHookReleaseForKernel(unload_allowed_uid, unload_allowed_hook);

	return SCE_KERNEL_STOP_SUCCESS;
}
