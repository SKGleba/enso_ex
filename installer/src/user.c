/* user.c -- module load workaround
 *
 * Copyright (C) 2017 molecule, 2018-2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/kernel/modulemgr.h>

#include "enso.h"

// user library as a workaround to load kernel module at runtime

int ensoCheckMBR(void) {
	return k_ensoCheckMBR();
}

int ensoCheckBlocks(void) {
	return k_ensoCheckBlocks();
}

int ensoWriteConfig(void) {
	return k_ensoWriteConfig();
}

int ensoWriteBlocks(void) {
	return k_ensoWriteBlocks();
}

int ensoWriteMBR(void) {
	return k_ensoWriteMBR();
}

int ensoCheckRealMBR(void) {
	return k_ensoCheckRealMBR();
}

int ensoUninstallMBR(void) {
	return k_ensoUninstallMBR();
}

int ensoCleanUpBlocks(void) {
	return k_ensoCleanUpBlocks();
}

int ensoWriteRecoveryConfig(void) {
	return k_ensoWriteRecoveryConfig();
}

int ensoWriteRecoveryBlob(void) {
	return k_ensoWriteRecoveryBlob();
}

int ensoWriteRecoveryMbr(void) {
	return k_ensoWriteRecoveryMbr();
}

int module_start(int args, void *argv) {
	(void)args;
	(void)argv;
	return SCE_KERNEL_START_SUCCESS;
}
void _start() __attribute__ ((weak, alias ("module_start")));

int module_stop() {
	return SCE_KERNEL_STOP_SUCCESS;
}
