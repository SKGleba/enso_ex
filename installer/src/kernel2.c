#include <psp2kern/kernel/modulemgr.h>
#include <stdio.h>
#include <string.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/threadmgr.h>

#include <taihen.h>

int siofix(void *func) {
	int ret = 0;
	int res = 0;
	int uid = 0;
	ret = uid = ksceKernelCreateThread("siofix", func, 64, 0x10000, 0, 0, 0);
	if (ret < 0){ret = -1; goto cleanup;}
	if ((ret = ksceKernelStartThread(uid, 0, NULL)) < 0) {ret = -1; goto cleanup;}
	if ((ret = ksceKernelWaitThreadEnd(uid, &res, NULL)) < 0) {ret = -1; goto cleanup;}
	ret = res;
cleanup:
	if (uid > 0) ksceKernelDeleteThread(uid);
	return ret;}

static void *pa2va(unsigned int pa) {
	unsigned int va = 0;
	unsigned int vaddr;
	unsigned int paddr;
	unsigned int i;

	for (i = 0; i < 0x100000; i++) {
		vaddr = i << 12;
		__asm__("mcr p15,0,%1,c7,c8,0\n\t"
				"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));
		if ((pa & 0xFFFFF000) == (paddr & 0xFFFFF000)) {
			va = vaddr + (pa & 0xFFF);
			break;
		}
	}
	return (void *)va;
}

static tai_hook_ref_t unload_allowed_hook;
static SceUID unload_allowed_uid;

int unload_allowed_patched(void) {
	TAI_CONTINUE(int, unload_allowed_hook);
	return 1; // always allowed
}

int checkfw(void) {
	int fd;
	unsigned int *tbl = NULL;
	void *vaddr;
	unsigned int paddr;
	unsigned int i;
	int write;
	unsigned int btw;
	unsigned int ttbr0;
	__asm__ volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbr0));
	tbl = (unsigned int *) pa2va(ttbr0 & ~0xFF);
	fd = ksceIoOpen("ur0:temp/temp.t", SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	if (fd < 0)
		return 0;
	i = 0x1F8;
	tbl[0x3E0] = (i << 20) | 0x10592;
	vaddr = &tbl[0x3E0];
	__asm__ volatile ("dmb sy");
	__asm__ volatile ("mcr p15,0,%0,c7,c14,1" :: "r" (vaddr) : "memory");
	__asm__ volatile ("dsb sy\n\t"
			"mcr p15,0,r0,c8,c7,0\n\t"
			"dsb sy\n\t"
			"isb sy" ::: "memory");
	vaddr = (void *) 0x3E000000;
	__asm__ volatile ("mcr p15,0,%1,c7,c8,0\n\t"
			"mrc p15,0,%0,c7,c4,0\n\t" : "=r" (paddr) : "r" (vaddr));
	ksceIoWrite(fd, (void *)vaddr + 0x50044, 3);
	ksceIoClose(fd);
	fd = ksceIoOpen("ur0:temp/temp.t", SCE_O_RDONLY, 0777);
	static char ubuf[3];
	ksceIoRead(fd, ubuf, 3);
	static char fwchkloc[14];
	sprintf(fwchkloc, "ur0:temp/%s.t", ubuf);
	ksceIoClose(fd);
	ksceIoRename("ur0:temp/temp.t", fwchkloc);
	return 1;
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

	int ret = 0, state = 0;
	ENTER_SYSCALL(state);
	siofix(checkfw);
	EXIT_SYSCALL(state);
	
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
