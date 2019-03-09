/*
	gamecard-microsd
	Copyright 2017-2018, xyz

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/threadmgr.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

static SceUID hookid = -1;
static tai_hook_ref_t ksceVfsNodeInitializePartitionRef;

static int ksceVfsNodeInitializePartitionPatched(int *node, int *new_node_p, void *opt, int flags) {
  int res = TAI_CONTINUE(int, ksceVfsNodeInitializePartitionRef, node, new_node_p, opt, flags);

  if (res == 0 && new_node_p) {
    int *new_node = (int *)*new_node_p;
    int *mount = (int *)new_node[19];
    mount[20] &= ~0x10000;
  }

  return res;
}

tai_hook_ref_t hook_get_partition;
tai_hook_ref_t hook_write;
tai_hook_ref_t hook_mediaid;

uint32_t magic = 0x7FFFFFFF;

void *sdstor_mediaid;

void *my_get_partition(const char *name, size_t len) {
	void *ret = TAI_CONTINUE(void*, hook_get_partition, name, len);
	if (!ret && len == 18 && strcmp(name, "gcd-lp-act-mediaid") == 0) {
		return &magic;
	}
	return ret;
}

uint32_t my_write(uint8_t *dev, void *buf, uint32_t sector, uint32_t size) {
	if (dev[36] == 1 && sector == magic) {
		return 0;
	}
	return TAI_CONTINUE(uint32_t, hook_write, dev, buf, sector, size);
}

uint32_t my_mediaid(uint8_t *dev) {
	uint32_t ret = TAI_CONTINUE(uint32_t, hook_mediaid, dev);

	if (dev[36] == 1) {
		memset(dev + 20, 0xFF, 16);
		memset(sdstor_mediaid, 0xFF, 16);

		return magic;
	}
	return ret;
}

void patch_sdstor() {
	tai_module_info_t sdstor_info;
	sdstor_info.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &sdstor_info) < 0)
		return;

	module_get_offset(KERNEL_PID, sdstor_info.modid, 1, 0x1720, (uintptr_t *) &sdstor_mediaid);

	taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_get_partition, sdstor_info.modid, 0, 0x142C, 1, my_get_partition);
	taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_write, sdstor_info.modid, 0, 0x2C58, 1, my_write);
	taiHookFunctionOffsetForKernel(KERNEL_PID, &hook_mediaid, sdstor_info.modid, 0, 0x3D54, 1, my_mediaid);
}
	

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	static char uwu[4] = {0x30, 0x30, 0x30, 0};
	SceUID fd = ksceIoOpen("sa0:eex/mmt_cfg.bin", SCE_O_RDONLY, 0);
	if (fd < 0)
		return SCE_KERNEL_START_FAILED;
	ksceIoRead(fd, uwu, 4);
	ksceIoClose(fd);
	
	ksceIoMount(0x800, NULL, 0, 0, 0, 0);
	
	if (uwu[0] == 0x31) patch_sdstor();
	if (uwu[1] == 0x31) hookid = taiHookFunctionExportForKernel(KERNEL_PID, &ksceVfsNodeInitializePartitionRef, "SceIofilemgr", TAI_ANY_LIBRARY, 0xA5A6A55C, ksceVfsNodeInitializePartitionPatched);
	if (uwu[2] == 0x31) ksceKernelDelayThread(uwu[3] * 1000 * 1000);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	return SCE_KERNEL_STOP_SUCCESS;
}
