// This code will "clone" EMMC to GCSD.

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdkkern.h>

#define SKIP (0)
#define OFFSET (0)
#define CPYSIZE (*(uint32_t *)(mbr + 0x24))

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	void *fsp_buf;
	char mbr[0x200];
	int mblk = ksceKernelAllocMemBlock("clone_buf", 0x10C0D006, 0x1000000, NULL); // 16MiB memblock
	if (mblk < 0)
		return SCE_KERNEL_START_FAILED;
	ksceKernelGetMemBlockBase(mblk, (void**)&fsp_buf);
	
	int sd = ksceSdifGetSdContextPartValidateSd(1), mmc = ksceSdifGetSdContextPartValidateMmc(0);
	
	if (sd == 0 || mmc == 0) // make sure that both devices are present
		return SCE_KERNEL_START_FAILED;
	
	ksceSdifReadSectorMmc(mmc, 0, &mbr, 1);
	uint32_t copied = SKIP, size = CPYSIZE, off = OFFSET; // get the emmc's size
	if (ksceSdifWriteSectorSd(sd, 0, &mbr, 1) < 0) // if for some reason it fails to write to the sd card, abort
		return SCE_KERNEL_START_FAILED;
	
	while((copied + 0x8000) <= size) { // first copy with full buffer
		ksceSdifReadSectorMmc(mmc, off + copied, fsp_buf, 0x8000);
		ksceSdifWriteSectorSd(sd, off + copied, fsp_buf, 0x8000);
		copied = copied + 0x8000;
	}
	
	if (copied < size && (size - copied) <= 0x8000) { // then copy the remaining data
		ksceSdifReadSectorMmc(mmc, off + copied, fsp_buf, (size - copied));
		ksceSdifWriteSectorSd(sd, off + copied, fsp_buf, (size - copied));
		copied = size;
	}
	
	// no memblock free because my compiler craps (lulx)
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	return SCE_KERNEL_STOP_SUCCESS;
}