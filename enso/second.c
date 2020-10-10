/* second.c -- bootloader patches
 *
 * Copyright (C) 2017 molecule, 2018-2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>

#ifdef FW_360
	#include "360/nsbl.h"
#else
	#include "365/nsbl.h"
#endif

#include "bm_compat.h"
#include "bm_compat.c"
#include "ex_defs.h"

#define unlikely(expr) __builtin_expect(!!(expr), 0)

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
	
#define NSKBL_EXPORTS(num) (NSKBL_EXPORTS_ADDR + (num * 4))
	
static const char *new_psp2bcfg = E2X_CKLDR_NAME;

static int icsahb = 0; // invalid self flag

// sdif globals
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void *(*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;

static void **get_export_func(SceModuleObject *mod, uint32_t lib_nid, uint32_t func_nid) {
    for (SceModuleExports *ent = mod->ent_top_user; ent != mod->ent_end_user; ent++) {
        if (ent->lib_nid == lib_nid) {
            for (int i = 0; i < ent->num_functions; i++) {
                if (ent->nid_table[i] == func_nid) {
                    return &ent->entry_table[i];
                }
            }
        }
    }
    return NULL;
}

static int is_cable(void) {
    SceBootArgs *boot_args = (*sysroot_ctx_ptr)->boot_args;
    if (boot_args->field_D8 == 0xC) {
        return 1;
    } else {
        if (boot_args->field_D8 == 0x8) {
			return 1;
		} else {
			return 0;
		}
    }
}

static int self_auth_header_patched(void *myaddr, int a1, int a2, int a3) {
	int ret = self_auth_header(1, a1, a2, a3);
	DACR_OFF(
		icsahb = (ret < 0);
    );
	if (icsahb) {
		*(uint32_t *)((void *)(a3) + 0xa8) = 0x40;
		ret = 0;
	}
	return ret;
}

static int self_setup_authseg_patched(void *myaddr, int a1) {
	if (icsahb)
		return 2;
	else
		return self_setup_authseg(1, a1);
}

static int self_load_block_patched(void *myaddr, int a1, int a2) {
	if (icsahb)
		return 0;
	else
		return self_load_block(1, a1, a2);
}

static void allow_fselfs() {
	*(uint16_t *)SGB_BOFF = 0xd162; // redirect argerrorh to another one (3x4bytes free for patched func offsets)
	*(uint32_t *)(SGB_BOFF + 0x66) = 0x47804813; // bl self_auth_header -> ldr r0, argerrorh[0]; blx r0
	*(uint32_t *)(SGB_BOFF + 0x76) = 0x47804810; // bl self_setup_authseg -> ldr r0, argerrorh[1]; blx r0
	*(uint32_t *)(SGB_BOFF + 0x86) = 0x4780480d; // bl self_load_block -> ldr r0, argerrorh[2]; blx r0
	*(uint32_t *)(SGB_BOFF + 0xB6) = (uint32_t)self_auth_header_patched; // 
	*(uint32_t *)(SGB_BOFF + 0xBA) = (uint32_t)self_setup_authseg_patched; // 3x4 bytes of unused arg error handler
	*(uint32_t *)(SGB_BOFF + 0xBE) = (uint32_t)self_load_block_patched; // 
	clean_dcache((void *)SGB_ROFF, 0x100);
	flush_icache();
}

// sdif patches for MBR redirection
static int sdif_read_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
    int ret;
#ifndef NO_MBR_REDIRECT
    if (unlikely(sector == 0 && nSectors > 0)) {
        if (get_sd_context_part_validate_mmc(0) == ctx) {
            ret = sdif_read_sector_mmc(ctx, 1, buffer, 1);
            if (ret >= 0 && nSectors > 1) {
                ret = sdif_read_sector_mmc(ctx, 1, buffer + 0x200, nSectors-1);
            }
            return ret;
        }
    }
#endif

    return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

// Read device to a memblock, mark as RX if req, return ptr (only FAT16 FS)
static void *load_device(uint32_t device, char *blkname, uint32_t offblk, uint32_t szblk, int type, int mode) {
	int mblk = sceKernelAllocMemBlock(blkname, 0x1020D006, szblk * 0x200, NULL);
	if (mblk >= 0) {
		void *ybas = NULL;
		sceKernelGetMemBlockBase(mblk, (void **)&ybas);
		if (((type) ? read_sector_sd((int *)*(uint32_t *)device, offblk, (int)ybas, szblk) : read_block_os0(device, offblk, szblk, ybas)) >= 0) {
			if (mode)
				sceKernelRemapBlock(mblk, 0x1020D005);
			clean_dcache(ybas, szblk * 0x200);
			flush_icache();
			return ybas;
		}
		sceKernelFreeMemBlock(mblk);
	}
	return NULL;
}

// Read file to a memblock/buf, mark as RX if req, return ptr (only FAT16 FS, max sz 8MB)
static void *load_file(char *fpath, char *blkname, unsigned int size, int mode) {
	long long int Var11 = iof_open(fpath, 1, 0);
	if (Var11 >= 0) {
		uint32_t uVar4 = (uint32_t)((unsigned long long int)Var11 >> 0x20);
		Var11 = iof_get_sz(uVar4, (int)Var11, 0, 0, 2);
		unsigned int uVar7 = (size > 0) ? size : (unsigned int)((unsigned long long int)Var11 >> 0x20);
		if (mode > 1) { // direct/fixed sz mode
			if ((int)iof_get_sz(uVar4, (int)Var11, 0, 0, 0) >= 0 && iof_read(uVar4, (void *)blkname, &uVar7) >= 0) {
				if (*(uint32_t *)blkname == 0) { // assume that the file has some magic
					iof_close(uVar4);
					return (void *)2;
				}
				iof_close(uVar4);
				return NULL;
			}
			iof_close(uVar4);
			return (void *)3;
		}
		unsigned int blksz = 0x1000;
		while (blksz < uVar7 && blksz < 0x800000)
			blksz-=-0x1000;
		int mblk = sceKernelAllocMemBlock(blkname, 0x1020D006, blksz, NULL);
		void *xbas = NULL;
		sceKernelGetMemBlockBase(mblk, (void **)&xbas);
		if ((int)Var11 >= 0 && uVar7 < blksz && (int)iof_get_sz(uVar4, (int)Var11, 0, 0, 0) >= 0 && iof_read(uVar4, xbas, &uVar7) >= 0 && *(uint32_t *)xbas != 0) {
			iof_close(uVar4);
			if (mode) {
				sceKernelRemapBlock(mblk, 0x1020D005);
				clean_dcache(xbas, blksz);
				flush_icache();
			}
			return xbas;
		}
		iof_close(uVar4);
		sceKernelFreeMemBlock(mblk);
	}
	return NULL;
}

// Use GC-SD instead of emmc or GC-SD instead of os0
static void dnand(unsigned int ctrl) {
	int error = 1, (*rf)(void *kbl_param, unsigned int ctrldata) = NULL;
    void *rbase = NULL;
	syscon_common_write(1, 0x888, 2); // enable the GC slot
	*(uint16_t*)0x51001252 = 0x2500; // skip main emmc init
	*(uint32_t*)0x51167594 = *(uint32_t*)0x51167594 | 0x40000; // enable sd0 mounting
	clean_dcache((void *)0x51001250, 0x10);
	clean_dcache((void *)0x51167590, 0x10);
	flush_icache();
	setup_emmc(); // reinit main storages
	char mbr[0x200];
	if (read_sector_sd((int *)*(uint32_t *)0x5102801c, 0, (int)&mbr, 1) >= 0) {
		if (*(uint32_t *)mbr == 0x796e6f53) { // EMMC dump/SCE formatted GCSD, use its os0 as main os0
			gpio_port_set(0, 7);
			init_part((unsigned int *)0x51167784, 0x110000, (unsigned int *)0x510010C5, (unsigned int *)0x51028018);
			error = 0;
		} else if (*(uint32_t *)(mbr + 0x38) == 0x20363154) { // base+0x36 = 'FA[T16 ]  ' - fat16 partition, set as os0
			gpio_port_set(0, 7);
			init_part((unsigned int *)0x51167784, 0x100000, (unsigned int *)0x510010C5, (unsigned int *)0x51028018);
			// now there will be os0 and sd0 mounted with the same src
			error = 0;
		} else // unk magic
			error = 2;
	}
	while(error > 0) {
		syscon_common_read(&ctrl, 0x101);
		if ((CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_NOENT) && error == 1) // No SD found
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_SDERR) && error == 2)) // Incorrect SD magic
			break;
	}
	gpio_port_clear(0, 7);
}
	
// Read and run our recovery code from GC-SD
static void recovery(unsigned int ctrl) {
	int error = 1, (*rf)(void *kbl_param, unsigned int ctrldata) = NULL;
    void *rbase = NULL;
	syscon_common_write(1, 0x888, 2); // enable the GC slot
	*(uint16_t*)0x51001252 = 0x2500; // skip main emmc init
	*(uint32_t*)0x51167594 = *(uint32_t*)0x51167594 | 0x40000; // enable sd0 mounting
	clean_dcache((void *)0x51001250, 0x10);
	clean_dcache((void *)0x51167590, 0x10);
	flush_icache();
	setup_emmc(); // reinit main storages
	RecoveryBlockStruct recoveryblock;
	if (read_sector_sd((int *)*(uint32_t *)0x5102801c, E2X_RECOVERY_BLKOFF, (int)&recoveryblock, 1) >= 0) {
		if (recoveryblock.magic == E2X_MAGIC) { // custom-formatted GCSD
			if (recoveryblock.flags[0] == 1) // use GCSD os0 as main os0
				init_part((unsigned int *)0x51167784, 0x110000, (unsigned int *)0x510010C5, (unsigned int *)0x51028018);
			if (recoveryblock.flags[1] == 1) { // use the flashed recovery
				gpio_port_set(0, 7);
				rf = (void *)(load_device(0x5102801c, "recovery", recoveryblock.blkoff, recoveryblock.blksz, 1, 1) + 1);
				error = (rf != (void *)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 2;
			}
		} else if (*(uint32_t *)(recoveryblock.data + 0x28) == 0x20363154) { // base+0x36 = 'FA[T16 ]  ' - fat16 partition 
			gpio_port_set(0, 7);
			rf = (void *)(load_file("sd0:" E2X_RECOVERY_FNAME, "recovery", 0, 1) + 1);
			error = (rf != (void *)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 1;
		}
	} else { // use the recovery in os0 (GCSD probs)
		gpio_port_set(0, 7);
		rf = (void *)(load_file("os0:" E2X_RECOVERY_FNAME, "recovery", 0, 1) + 1);
		error = (rf != (void *)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 1;
	}
	while(error > 0) {
		syscon_common_read(&ctrl, 0x101);
		if ((CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_NOENT) && error == 1) // No recovery found
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_SDERR) && error == 2) // Error running recovery
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RETERR) && error == 4)) // Recovery returned !0
			break;
	}
	gpio_port_clear(0, 7);
}

// patch get_hwcfg to give important offs
static int get_hwcfg_patched(uint32_t *dst) {
	if (dst[0] == 69) {
		syscon_common_read(&dst[0], 0x101);
		dst[1] = (uint32_t)load_file;
		dst[2] = (uint32_t)(*sysroot_ctx_ptr)->boot_args;
		dst[3] = (uint32_t)NSKBL_EXPORTS_ADDR;
		return 0;
	} else
		return get_hwcfg(dst);
}

// Run BootMgr and resume psp2bootconfig load
static int load_psp2bootconfig_patched(uint32_t myaddr, int *uids, int count, int osloc, int unk) {
	allow_fselfs();
	*(uint32_t *)NSKBL_EXPORTS(26) = (uint32_t)get_hwcfg_patched;
	void (*tcode)(void) = (void *)(load_file("os0:" E2X_BOOTMGR_NAME, "bootmgr_ex", 0, 1) + 1);
	if (tcode != (void *)0x1)
		tcode();
	myaddr = (uint32_t)new_psp2bcfg;
	return module_load_direct((SceModuleLoadList *)&myaddr, uids, count, osloc, unk);
}

// main function to hook stuff
#define HOOK_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    DACR_OFF(                                               \
        name = *func;                                       \
        *func = name ## _patched;                           \
    );                                                      \
} while (0)
#define FIND_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    DACR_OFF(                                               \
        name = *func;                                       \
    );                                                      \
} while (0)
static int module_load_patched(const SceModuleLoadList *list, int *uids, int count, int unk) {
    int ret;
    SceObject *obj;
    SceModuleObject *mod;
    int sdif_idx = -1;
	
    for (int i = 0; i < count; i-=-1) {
        if (!list[i].filename) {
            continue; // wtf sony why don't you sanitize input
        }
        if (strncmp(list[i].filename, "sdif.skprx", 10) == 0)
            sdif_idx = i; // never skip MBR redirection patches
    }
	
	ret = module_load(list, uids, count, unk);
	
	// skip all unclean uids
	for (int i = 0; i < count; i-=-1) {
		if (uids[i] < 0)
			uids[i] = 0;
    }
	
    // patch sdif
    if (sdif_idx >= 0) {
        obj = get_obj_for_uid(uids[sdif_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            HOOK_EXPORT(sdif_read_sector_mmc, 0x96D306FA, 0x6F8D529B);
            FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
        }
    }
	
    return ret;
}
#undef HOOK_EXPORT
#undef FIND_EXPORT

void go(void) {
	gpio_port_clear(0, 7);
	
	// remove loaderr (uids can be unclean now)
	*(uint32_t *)0x510014ee = 0xbf00bf00;
	clean_dcache((void *)0x510014e0, 0x20);
	flush_icache();
	
	// redirect module_load_from_list
	*(uint32_t *)NSKBL_EXPORTS(7) = (uint32_t)module_load_patched;
	
	unsigned int ctrl;
	syscon_common_read(&ctrl, 0x101);
	
	// redirect load_psp2bootconfig
	if (!CTRL_BUTTON_HELD(ctrl, E2X_IPATCHES_SKIP)) {
		*(uint32_t *)0x51001688 = 0x47806800;
		*(uint32_t *)PSP2BCFG_STRING_ADDR = (uint32_t)load_psp2bootconfig_patched;
		clean_dcache((void *)PSP2BCFG_STRING_ADDR, 0x10);
		clean_dcache((void *)0x51001680, 0x10);
		flush_icache();
	}
	
	// Recovery if wall-connected & SELECT/START held
	if (is_cable()) {
		if (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RUNDF))
			recovery(ctrl);
		else if (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RUNDN))
			dnand(ctrl);
	}
}

__attribute__ ((section (".text.start"))) void start(void) {
    go();
}
