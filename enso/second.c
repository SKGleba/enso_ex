/* second.c -- bootloader patches
 *
 * Copyright (C) 2017 molecule
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

#define INSTALL_HOOK_THUMB(func, addr) \
do {                                                \
    unsigned *target;                                 \
    target = (unsigned*)(addr);                       \
    *target++ = 0xC004F8DF; /* ldr.w    ip, [pc, #4] */ \
    *target++ = 0xBF004760; /* bx ip; nop */          \
    *target = (unsigned)func;                         \
} while (0)

#define INSTALL_RET_THUMB(addr, ret)   \
do {                                   \
    unsigned *target;                  \
    target = (unsigned*)(addr);        \
    *target = 0x47702000 | (ret); /* movs r0, #ret; bx lr */ \
} while (0)

// sdif globals
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void *(*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;

// patches globals
static int g_pload = 0;
static void *tpatches[15];

// sigpatch globals
static int g_sigpatch_disabled = 0;
static int g_homebrew_decrypt = 0;
static int (*sbl_parse_header)(uint32_t ctx, const void *header, int len, void *args) = NULL;
static int (*sbl_set_up_buffer)(uint32_t ctx, int segidx) = NULL;
static int (*sbl_decrypt)(uint32_t ctx, void *buf, int sz) = NULL;

// sysstate final function
static void __attribute__((noreturn)) (*sysstate_final)(void) = NULL;

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

static int is_safe_mode(void) {
    SceBootArgs *boot_args = (*sysroot_ctx_ptr)->boot_args;
    uint32_t v;
    if (boot_args->debug_flags[7] != 0xFF) {
        return 1;
    }
    v = boot_args->boot_type_indicator_2 & 0x7F;
    if (v == 0xB || (v == 4 && boot_args->resume_context_addr)) {
        v = ~boot_args->field_CC;
        if (((v >> 8) & 0x54) == 0x54 && (v & 0xC0) == 0) {
            return 1;
        } else {
            return 0;
        }
    } else if (v == 4) {
        return 0;
    }
    if (v == 0x1F || (uint32_t)(v - 0x18) <= 1) {
        return 1;
    } else {
        return 0;
    }
}

static int is_update_mode(void) {
    SceBootArgs *boot_args = (*sysroot_ctx_ptr)->boot_args;
    if (boot_args->debug_flags[4] != 0xFF) {
        return 1;
    } else {
        return 0;
    }
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

static inline int skip_patches(void) {
    return is_safe_mode() || is_update_mode();
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

// sigpatches for bootup
static int sbl_parse_header_patched(uint32_t ctx, const void *header, int len, void *args) {
    int ret = sbl_parse_header(ctx, header, len, args);
    if (unlikely(!g_sigpatch_disabled)) {
        DACR_OFF(
            g_homebrew_decrypt = (ret < 0);
        );
        if (g_homebrew_decrypt) {
            *(uint32_t *)(args + SBLAUTHMGR_OFFSET_PATCH_ARG) = 0x40;
            ret = 0;
        }
    }
    return ret;
}

static int sbl_set_up_buffer_patched(uint32_t ctx, int segidx) {
    if (unlikely(!g_sigpatch_disabled)) {
        if (g_homebrew_decrypt) {
            return 2; // always compressed!
        }
    }
    return sbl_set_up_buffer(ctx, segidx);
}

static int sbl_decrypt_patched(uint32_t ctx, void *buf, int sz) {
    if (unlikely(!g_sigpatch_disabled)) {
        if (g_homebrew_decrypt) {
            return 0;
        }
    }
    return sbl_decrypt(ctx, buf, sz);
}

static void __attribute__((noreturn)) sysstate_final_hook(void) {

    DACR_OFF(
        g_sigpatch_disabled = 1;
    );

    sysstate_final();
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

// Read file to a memblock, mark as RX if req, return ptr (only FAT16 FS, max sz 8MB)
static void *load_file(char *fpath, char *blkname, int mode) {
	long long int Var11 = iof_open(fpath, 1, 0);
	if (Var11 >= 0) {
		uint32_t uVar4 = (uint32_t)((unsigned long long int)Var11 >> 0x20);
		Var11 = iof_get_sz(uVar4, (int)Var11, 0, 0, 2);
		unsigned int uVar7 = (unsigned int)((unsigned long long int)Var11 >> 0x20);
		unsigned int blksz = 0x1000;
		while (blksz < uVar7 && blksz < 0x800000)
			blksz-=-0x1000;
		int mblk = sceKernelAllocMemBlock(blkname, 0x1020D006, blksz, NULL);
		void *xbas = NULL;
		sceKernelGetMemBlockBase(mblk, (void **)&xbas);
		if ((int)Var11 >= 0 && uVar7 < blksz && (int)iof_get_sz(uVar4, (int)Var11, 0, 0, 0) >= 0 && iof_read(uVar4, xbas, &uVar7) >= 0 && *(uint32_t *)xbas != 0) {
			iof_close(uVar4);
			if (mode)
				sceKernelRemapBlock(mblk, 0x1020D005);
			clean_dcache(xbas, blksz);
            flush_icache();
			return xbas;
		}
		iof_close(uVar4);
		sceKernelFreeMemBlock(mblk);
	}
	return NULL;
}

// Use GC-SD instead of emmc (use the int2ext patch with it)
static void dnand(unsigned int ctrl) {
	int error = 1, (*rf)(void *kbl_param, unsigned int ctrldata) = NULL;
    void *rbase = NULL;
	syscon_common_write(1, 0x888, 2); // enable the GC slot
	*(uint16_t*)0x51001252 = 0x2500; // skip main emmc init
	*(uint32_t*)0x51167594 = *(uint32_t*)0x51167594 | 0x40000; // enable sd0 mounting
	setup_emmc(); // reinit main storages
	char mbr[0x200];
	if (read_sector_sd((int *)*(uint32_t *)0x5102801c, 0, (int)&mbr, 1) >= 0) {
		if (*(uint32_t *)mbr == 0x796e6f53) { // EMMC dump/SCE formatted GCSD, use its os0 as main os0
			gpio_port_set(0, 7);
			init_part((unsigned int *)0x51167784, 0x110000, (unsigned int *)0x510010C5, (unsigned int *)0x51028018);
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
			} else { // use the recovery in os0 (GCSD probs)
				gpio_port_set(0, 7);
				rf = (void *)(load_file("os0:" E2X_RECOVERY_FNAME, "recovery", 1) + 1);
				error = (rf != (void *)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 3;
			}
		} else if (recoveryblock.magic != 0x796e6f53) { // make sure its not SCE MBR, assume its FAT16 and try to run recovery from it
			gpio_port_set(0, 7);
			rf = (void *)(load_file("sd0:" E2X_RECOVERY_FNAME, "recovery", 1) + 1);
			error = (rf != (void *)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 1;
		}
	}
	while(error > 0) {
		syscon_common_read(&ctrl, 0x101);
		if ((CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_NOENT) && error == 1) // No recovery found
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_SDERR) && error == 2) // Error running GC-SD recovery
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_OSERR) && error == 3) // Error running os0 recovery
		|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RETERR) && error == 4)) // Recovery returned !0
			break;
	}
	gpio_port_clear(0, 7);
}

// Read the patches file for code blobs and alloc them
static int load_patches_data(char *fpath) {
	void *patches = load_file(fpath, "patches", 0);
	if (patches == NULL)
		return 1;
	PayloadsBlockStruct *pstart = patches;
	if (pstart->magic != E2X_MAGIC)
		return 1;
	int mblk;
	unsigned int blksz;
	void *xbas = NULL;
	int i = 0;
	for (int i = 0; i < (E2X_MAX_EPATCHES_N - 1); i-=-1) {
		if (pstart->off[i] == 0)
			break;
		blksz = 0, mblk = 0;
		xbas = NULL;
		while (blksz < pstart->sz[i] && blksz < 0x800000)
			blksz-=-0x2000;
		mblk = sceKernelAllocMemBlock("", 0x1020D006, blksz, NULL);
		sceKernelGetMemBlockBase(mblk, (void **)&xbas);
		memcpy(xbas, (patches + pstart->off[i]), pstart->sz[i]);
		sceKernelRemapBlock(mblk, 0x1020D005);
		clean_dcache(xbas, blksz);
        flush_icache();
		tpatches[i] = (xbas + 1);
	}
	return 0;
}

// Run os0:patches.e2xd
static void add_tparty_patches(const SceModuleLoadList *list, int *uids, int count, int safemode, unsigned int ctrldata) {
	PayloadArgsStruct pargs;
	pargs.list = list; // --
	pargs.uids = uids; // pass module_load args
	pargs.count = count; // --
	pargs.safemode = safemode; // pass updatemode/safemode status
	pargs.ctrldata = ctrldata; // pass buttons status
	void (*runtp)(PayloadArgsStruct *pargs);
	gpio_port_set(0, 7);
	if (g_pload == 0) {
		DACR_OFF(
			g_pload = 34;
			if (load_patches_data("os0:" E2X_IPATCHES_FNAME) == 1) // load the data file with blobs
				tpatches[0] = (void *)(load_file("os0:patches.e2xp", "patches", 1) + 1); // if blobs load fails, try to load the old patches file
			if (tpatches[0] != (void *)0x1)
				g_pload = 1; // first run
		);
	} else {
		DACR_OFF(
			g_pload = g_pload + 1;
		);
	}
	pargs.trun = g_pload; // pass run count, thats because the modload func is re-run a few times
	if (g_pload < 34) {
		for (int i = 0; i < (E2X_MAX_EPATCHES_N - 1); i-=-1) {
			if ((tpatches[i] == NULL) || (tpatches[i] == (void *)0x1))
				break;
			runtp = tpatches[i];
			runtp(&pargs);
		}
	}
	gpio_port_clear(0, 7);
}

// Run BootMgr and resume psp2bootconfig load
static int load_psp2bootconfig_patched(uint32_t myaddr, int *uids, int count, int osloc, int unk) {
	void (*tcode)(void) = (void *)(load_file("os0:" E2X_BOOTMGR_NAME, "bootmgr_ex", 1) + 1);
	if (tcode != (void *)0x1)
		tcode();
	myaddr = PSP2BOOTCONFIG_STRING;
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
	unsigned int ctrldata;
	syscon_common_read(&ctrldata, 0x101); // TODO: don't read at every modload, find a way to make it a global.
    int sdif_idx = -1, authmgr_idx = -1, sysstate_idx = -1;
    int skip = skip_patches(); // [safe] or [update] mode flag
	int noap = CTRL_BUTTON_HELD(ctrldata, E2X_IPATCHES_SKIP); // skip custom patches (patches.e2xp) flag
	if (is_true_dolce())
		noap = skip;
	
    for (int i = 0; i < count; i-=-1) {
        if (!list[i].filename) {
            continue; // wtf sony why don't you sanitize input
        }
        if (strncmp(list[i].filename, "sdif.skprx", 10) == 0) {
            sdif_idx = i; // never skip MBR redirection patches
        } else if (strncmp(list[i].filename, "authmgr.skprx", 13) == 0) {
            authmgr_idx = i;
        } else if (!skip && strncmp(list[i].filename, "sysstatemgr.skprx", 17) == 0) {
            sysstate_idx = i;
        }
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
	
    // patch authmgr
    if (authmgr_idx >= 0) {
        obj = get_obj_for_uid(uids[authmgr_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            HOOK_EXPORT(sbl_parse_header, 0x7ABF5135, 0xF3411881);
            HOOK_EXPORT(sbl_set_up_buffer, 0x7ABF5135, 0x89CCDA2C);
            HOOK_EXPORT(sbl_decrypt, 0x7ABF5135, 0xBC422443);
        }
    }
	
    // patch sysstate to load unsigned boot configs
    if (sysstate_idx >= 0) {
        obj = get_obj_for_uid(uids[sysstate_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            DACR_OFF(
                INSTALL_RET_THUMB(mod->segments[0].buf + SYSSTATE_IS_MANUFACTURING_MODE_OFFSET, 1);
                *(uint32_t *)(mod->segments[0].buf + SYSSTATE_IS_DEV_MODE_OFFSET) = 0x20012001;
                memcpy(mod->segments[0].buf + SYSSTATE_RET_CHECK_BUG, sysstate_ret_patch, sizeof(sysstate_ret_patch));
				memcpy(mod->segments[0].buf + SYSSTATE_SD0_STRING, ur0_path, sizeof(ur0_path));
				memcpy(mod->segments[0].buf + SYSSTATE_SD0_PSP2CONFIG_STRING, ur0_psp2config_path, sizeof(ur0_psp2config_path));
                // this patch actually corrupts two words of data, but they are only used in debug printing and seem to be fine
                INSTALL_HOOK_THUMB(sysstate_final_hook, mod->segments[0].buf + SYSSTATE_FINAL_CALL);
                sysstate_final = mod->segments[0].buf + SYSSTATE_FINAL;
            );
        }
    }
	
	// Add custom patches
	if (!noap)
		add_tparty_patches(list, uids, count, skip, ctrldata);
	
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
    *module_load_func_ptr = module_load_patched;
	
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
	if (is_cable() == 1) {
		if (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RUNDF))
			recovery(ctrl);
		else if (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RUNDN))
			dnand(ctrl);
	}
}

__attribute__ ((section (".text.start"))) void start(void) {
    go();
}
