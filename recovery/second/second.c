/* second.c -- bootloader patches, cleanup and recovery
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>

#include "../../core/enso.h"
#include "../../core/ex_defs.h"
#include "../../core/nskbl.h"

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

// --------------------

/*
	SDIF EMMC read/write patch
*/
// find func va by export nid
static void** get_export_func(SceModuleObject * mod, uint32_t lib_nid, uint32_t func_nid) {
	for (SceModuleExports* ent = mod->ent_top_user; ent != mod->ent_end_user; ent++) {
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

// replace func va for export nid
#define HOOK_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    DACR_OFF(                                               \
        name = *func;                                       \
        *func = name ## _patched;                           \
    );                                                      \
} while (0)

// find func va by export nid and assign to a global
#define FIND_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    DACR_OFF(                                               \
        name = *func;                                       \
    );                                                      \
} while (0)

// sdif globals
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void* (*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;
static int disable_bootarea_update = 0;
static unsigned int fakembr_offset = ENSO_EMUMBR_OFFSET;

// sdif patches for MBR redirection
static int sdif_read_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (unlikely(sector == 0 && nSectors > 0)) {
		if (get_sd_context_part_validate_mmc(0) == ctx) {
			int ret = sdif_read_sector_mmc(ctx, fakembr_offset, buffer, 1);
			if (ret >= 0 && nSectors > 1)
				ret = sdif_read_sector_mmc(ctx, 1, buffer + SDIF_SECTOR_SIZE, nSectors - 1);
			return ret;
		} else if ((uint32_t)ctx == E2X_READ_REAL_MBR_KEY) // use fake ctx as an indicator that we want to read the real mbr
			ctx = get_sd_context_part_validate_mmc(0);
	}
	
	return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

// block from writing boot sectors unless prev asked
static int sdif_write_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
    if (unlikely((uint32_t)ctx == E2X_BOOTAREA_LOCK_KEY) && (sector == E2X_BOOTAREA_LOCK_KEY)) {  // change lock mode
        DACR_OFF(
			disable_bootarea_update = nSectors;
		);
		return E2X_BOOTAREA_LOCK_CG_ACK;
    }
    if (((sector < SBLS_END && (sector >= SBLS_START || sector < IDSTOR_START)) && disable_bootarea_update) && (get_sd_context_part_validate_mmc(0) == ctx)) {
        return -1;
    }
	return sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
}

// patch sdif's emmc R/W, clean dirty uids
static int module_load_patched(const SceModuleLoadList* list, int* uids, int count, int unk) {

	// load req modules
	int ret = module_load(list, uids, count, unk);
	if (ret < 0)
		printf("x E: lload[%d] 0x%X\n", count, ret);

	// skip all unclean uids
	for (int i = 0; i < count; i -= -1) {
		if (uids[i] < 0) {
			printf("x W: dirty uid[%d]: %s\n", i, (list[i].filename) ? list[i].filename : "NULL");
			uids[i] = 0;
		} else
			printf("x mloaded [%d : 0x%X]: %s\n", i, uids[i], (list[i].filename) ? list[i].filename : "NULL");
	}

	// find and patch sdif
	for (int i = 0; i < count; i -= -1) {
		if (list[i].filename) {
			if (!(strncmp(list[i].filename, "sdif.skprx", 10))) {
				SceObject* obj = get_obj_for_uid(uids[i]);
				if (obj) {
					SceModuleObject* mod = (SceModuleObject*)&obj->data;
					HOOK_EXPORT(sdif_read_sector_mmc, 0x96D306FA, 0x6F8D529B);
					HOOK_EXPORT(sdif_write_sector_mmc, 0x96D306FA, 0x175543D2);
					FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
				} else
					printf("x E: no sdif\n");
				break;
			}
		} else
			printf("x W: module[%d] NULL\n", i);
	}

	return ret;
}
#undef HOOK_EXPORT
#undef FIND_EXPORT
// --------------------

/*
	FSELF patches for lkernel module loader
	STRUCT & FUNC LAYOUT MAY CHANGE WITH FIRMWARE VERSION
*/
static int icsahb = 0; // invalid self flag

static int self_auth_header_patched(void* myaddr, int a1, int a2, int a3) {
	int ret = self_auth_header(1, a1, a2, a3);
	DACR_OFF(
		icsahb = (ret < 0);
	);
	if (icsahb) {
		*(uint32_t*)((void*)(a3)+0xa8) = 0x40;
		ret = 0;
	}
	return ret;
}

static int self_setup_authseg_patched(void* myaddr, int a1) {
	if (icsahb)
		return 2;
	else
		return self_setup_authseg(1, a1);
}

static int self_load_block_patched(void* myaddr, int a1, int a2) {
	if (icsahb)
		return 0;
	else
		return self_load_block(1, a1, a2);
}

static void allow_fselfs() {
	*(uint16_t*)NSKBL_LOAD_SELF_COMPRESSED_CHECK = 0xd162; // redirect argerrorh to another one (3x4bytes free for patched func offsets)
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0x66) = 0x47804813; // bl self_auth_header -> ldr r0, argerrorh[0]; blx r0
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0x76) = 0x47804810; // bl self_setup_authseg -> ldr r0, argerrorh[1]; blx r0
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0x86) = 0x4780480d; // bl self_load_block -> ldr r0, argerrorh[2]; blx r0
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0xB6) = (uint32_t)self_auth_header_patched; // 
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0xBA) = (uint32_t)self_setup_authseg_patched; // 3x4 bytes of unused arg error handler
	*(uint32_t*)(NSKBL_LOAD_SELF_COMPRESSED_CHECK + 0xBE) = (uint32_t)self_load_block_patched; // 
	clean_dcache((void*)NSKBL_LOAD_SELF_FUNC, 0x100);
	flush_icache();
}
// --------------------

/*
	Storage utils
*/
static int get_file(char* file_path, void* buf, uint32_t read_size, uint32_t offset) {
	printf("x get_file [0x%X] @ %s\n", read_size, file_path);
	int fd = iof_open(file_path, 1, 0); // open as r/o
	if (fd < 0)
		return -1;
	
	if (!read_size)
		read_size = iof_lseek(fd, 0, 0, 0, 2); // seek to end
	
	if (!buf) {
		iof_close(fd);
		return (int)read_size;
	}
	
	if (!read_size || iof_lseek(fd, 0, offset, 0, 0) < 0) {
		iof_close(fd);
		return -2;
	}
	
	if (iof_read(fd, buf, read_size) < 0) {
		iof_close(fd);
		return -3;
	}

	iof_close(fd);

	return 0;
}
// --------------------

/*
    enso_ex init stuff
*/
// init os0 for nskbl
static int init_os0(uint32_t mbr_off, unsigned int* ctx, int is_scembr) {
    // patch MBR offset to emuMBR
    *(uint16_t*)NSKBL_INIT_DEV_SETPARAM_MBR_OFF = 0x2100 | (uint16_t)mbr_off;  // movs r1, mbr_off
    clean_dcache((void*)NSKBL_INIT_DEV_SETPARAM_MBR_OFF_CACHER, 0x20);
    flush_icache();

    // set emuMBR offset for the sdif patch
    DACR_OFF(fakembr_offset = mbr_off;);

    // init os0
    int ret = init_part((unsigned int*)NSKBL_PARTITION_OS0, 0x100000 | (is_scembr ? 0x10000 : 0), (unsigned int*)read_sector_default, ctx);
    printf("x init_os0[%d|%08X]: 0x%08X\n", mbr_off, ctx, ret);

    // TODO: what do these do? but we need them for some reason
    *(uint32_t*)(NSKBL_PARTITION_OS0 + 0x2C) = 0;
    *(uint32_t*)(NSKBL_PARTITION_OS0 + 0x78) = 0x1A000100;
    *(uint32_t*)(NSKBL_PARTITION_OS0 + 0x84) = 0x1A001000;
    *(uint32_t*)(NSKBL_PARTITION_OS0 + 0x90) = 0x0001002B;

    return ret;
}
// --------------------

/*
	Custom kernel loader patches
*/
// custom get_hwcfg to give ckldr important offsets/data
static int get_hwcfg_patched(uint32_t* dst) {
    if (dst[0] == E2X_MAGIC) {
        patchedHwcfgStruct* expp = (void*)dst;
        expp->ex_ports.ctrl = (*sysroot_ctx_ptr)->boot_args->field_CC;
        expp->ex_ports.nskbl_exports_start = (void*)NSKBL_EXPORTS_ADDR;
        expp->ex_ports.get_file = get_file;
        expp->ex_ports.memcpy = memcpy;
        expp->ex_ports.memset = memset;
        expp->ex_ports.get_obj_for_uid = (void*)get_obj_for_uid;
        expp->ex_ports.alloc_memblock = (void*)sceKernelAllocMemBlock;
        expp->ex_ports.get_memblock = sceKernelGetMemBlockBase;
        expp->ex_ports.free_memblock = sceKernelFreeMemBlock;
        expp->ex_ports.module_dir = (char*)NSKBL_LMODLOAD_DIR;
        expp->ex_ports.kbl_param = (void*)(*sysroot_ctx_ptr)->boot_args;
        expp->ex_ports.protect_boot = &disable_bootarea_update;
        expp->ex_ports.init_os0 = init_os0;
        expp->ex_ports.printf = printf;
        return E2X_MAGIC;
    } else
        return get_hwcfg((void*)dst);
}

// Run BootMgr and resume psp2bootconfig load with our custom loader
static int load_psp2bootconfig_patched(uint32_t myaddr, int* uids, int count, int osloc, int unk) {
	printf("x @stage3\n");
	
	allow_fselfs();

	*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N) = (uint32_t)get_hwcfg_patched;
	if (!get_file("os0:" E2X_BOOTMGR_NAME, (void*)E2X_BOOTMGR_PADDR, 0, 0)) {
        printf("x bootmgr\n");
        int (*tcode)(uint32_t get_info_va) = (void*)(E2X_BOOTMGR_PADDR | 1);
		tcode((uint32_t)get_hwcfg_patched);
    }

	if (get_file("os0:" E2X_CKLDR_NAME, NULL, 0, 0) > 0)
		myaddr = (uint32_t)E2X_CKLDR_NAME;
	else {
		myaddr = (uint32_t)NSKBL_PSP2BCFG_STRING;
		printf("x W: no ckldr\n");
	}
	return module_load_direct((SceModuleLoadList*)&myaddr, uids, count, osloc, unk);
}
// --------------------

/*
    recovery
*/
// recovery from GC-SD
static int recovery_ccode(int *ctx, uint8_t *buf) {
	int ret = 0;
	RecoveryBlockStruct* rbr = (RecoveryBlockStruct*)buf;
	if (rbr->magic != E2X_MAGIC) {
        ret = read_sector_default_direct(ctx, E2X_RCONF_OFFSET, 1, (int)buf);
		if (ret < 0)
			return ret;
    }
    if (rbr->magic != E2X_MAGIC)
		return -1;

	return ((int (*)(int *, uint32_t))(buf + rbr->offset))(ctx, (uint32_t)get_hwcfg_patched);
}
// --------------------

// main
__attribute__((section(".text.start"))) void start(void* me) {

	printf("x @stage2 RECOVERY - patching nskbl\n");

    if (*(uint16_t*)NSKBL_LMODLOAD_CHKRET == 0xbf00) {
        if (!get_file("os0:rblob.e2xp", (void*)E2X_RBLOB_PADDR, 0, 0)) {
            printf("x guirecovery\n");
            void (*tcode)(uint32_t magic) = (void*)(E2X_RBLOB_PADDR | 1);
            clean_dcache((void*)E2X_RBLOB_PADDR, E2X_RBLOB_SIZE);
            flush_icache();
            tcode(E2X_MAGIC);
        } else
			printf("x W: no guirecovery\n");
		return;
    }

    // ignore module_load error (uids can be unclean now)
    *(uint16_t*)NSKBL_LMODLOAD_CHKRET = 0xbf00;
	*(uint16_t*)(NSKBL_LMODLOAD_CHKRET + 2) = 0xbf00;
	clean_dcache((void*)NSKBL_LMODLOAD_CHKRET_CACHER, 0x20);
	flush_icache();

	// use a custom module_load_from_list
	*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_LMODLOAD_N) = (uint32_t)module_load_patched;

	*(uint32_t*)NSKBL_LBOOTM_LPSP2BCFG = 0x47806800; // blx to psp2bootconfig string
	*(uint32_t*)NSKBL_PSP2BCFG_STRING_PTR = (uint32_t)load_psp2bootconfig_patched;
	clean_dcache((void*)NSKBL_LBOOTM_LPSP2BCFG_CACHER, 0x20);
	clean_dcache((void*)NSKBL_PSP2BCFG_STRING_PTR_CACHER, 0x20);
	flush_icache();

	if (recovery_ccode((int*)NSKBL_DEVICE_GCSD_CTX, (int*)E2X_RCONF_PADDR) < 0)
		printf("xR E: recovery_ccode failed\n");
    init_os0(ENSO_EMUMBR_OFFSET, (unsigned int*)NSKBL_DEVICE_EMMC_CTX, 1);

    printf("x resuming nskbl\n");

	// restore context and resume boot
	uint32_t* sp = *(uint32_t**)(ENSO_SP_AREA_OFFSET + ENSO_SP_TOP_CORE0); // sp top for core 0
	uint32_t* old_sp = sp - ENSO_SP_TOP_OLD_CORE0;

	// r0: 0x51167784 os0_dev
	// r1: 0xfffffffe
	// r2: sp - 0x110
	// r3: 0
	__asm__ volatile (
		"movw r0, #0x7784\n"
		"movt r0, #0x5116\n"
		"movw r1, #0xfffe\n"
		"movt r1, #0xffff\n"
		"mov r2, %0\n"
		"mov r3, #0\n"
		"mov sp, %1\n"
		"mov r4, %2\n"
		"bx r4\n" :: "r" (sp - 0x110), "r" (old_sp), "r" (0x5101F779) : "r0", "r1", "r2", "r3", "r4"
		);
}
// --------------------
