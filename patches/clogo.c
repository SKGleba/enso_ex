#include <inttypes.h>

#ifdef FW_360
	#include "../enso/360/nsbl.h"
#else
	#include "../enso/365/nsbl.h"
#endif

#include "../enso/bm_compat.h"
#include "../enso/ex_defs.h"

#define INSTALL_RET_THUMB(addr, ret)   \
do {                                   \
    unsigned *target;                  \
    target = (unsigned*)(addr);        \
    *target = 0x47702000 | (ret); /* movs r0, #ret; bx lr */ \
} while (0)
	
#define DACR_OFF(stmt)                 \
do {                                   \
    unsigned prev_dacr;                \
    __asm__ volatile(                  \
        "mrc p15, 0, %0, c3, c0, 0 \n" \
        : "=r" (prev_dacr)             \
    );                                 \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (0xFFFF0000)           \
    );                                 \
    stmt;                              \
    __asm__ volatile(                  \
        "mcr p15, 0, %0, c3, c0, 0 \n" \
        : : "r" (prev_dacr)            \
    );                                 \
} while (0)
	
#define DISPLAY_BOOT_LOGO_FUNC (0x6F9C)

// Read file to cSRAM, return ptr (only FAT16 FS)
static void *load_logo(char *fpath) {
	long long int Var11 = iof_open(fpath, 1, 0);
	if (Var11 >= 0) {
		uint32_t uVar4 = (uint32_t)((unsigned long long int)Var11 >> 0x20);
		Var11 = iof_get_sz(uVar4, (int)Var11, 0, 0, 2);
		unsigned int uVar7 = (unsigned int)((unsigned long long int)Var11 >> 0x20);
		SceKernelAllocMemBlockKernelOpt optp;
		optp.size = 0x58;
		optp.attr = 2;
		optp.paddr = 0x1c000000;
		int mblk = sceKernelAllocMemBlock("bootlogo", 0x6020D006, 0x200000, &optp);
		void *xbas = NULL;
		sceKernelGetMemBlockBase(mblk, (void **)&xbas);
		if ((int)Var11 >= 0 && uVar7 <= 0x200000 && (int)iof_get_sz(uVar4, (int)Var11, 0, 0, 0) >= 0 && iof_read(uVar4, xbas, &uVar7) >= 0 && *(uint32_t *)xbas != 0) {
			iof_close(uVar4);
			sceKernelFreeMemBlock(mblk);
			return xbas;
		}
		iof_close(uVar4);
		sceKernelFreeMemBlock(mblk);
	}
	return NULL;
}
	
void _start(PayloadArgsStruct *args) {
	if (args->trun != 2)
		return;
	SceObject *obj;
    SceModuleObject *mod;
	const SceModuleLoadList *list = args->list;
	for (int i = 0; i < args->count; i-=-1) {
		if (!list[i].filename)
            continue;
        if (!args->safemode && strncmp(list[i].filename, "display.skprx", 13) == 0) {
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				void *logo = load_logo("os0:bootlogo.raw");
				DACR_OFF(
					if (logo == NULL)
						INSTALL_RET_THUMB(mod->segments[0].buf + DISPLAY_BOOT_LOGO_FUNC, 0);
					else
						*(uint32_t *)(mod->segments[0].buf + 0x700e) = 0x20002000;
				);
			}
		}
    }
	return;
}