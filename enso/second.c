/* second.c -- bootloader patches and recovery
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>

#include "nsbl.h"
#include "enso.h"

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

/*
	SDIF read/write emmc patch
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
		} else if (ctx == E2X_READ_REAL_MBR_KEY) // use fake ctx as an indicator that we want to read the real mbr
			ctx = get_sd_context_part_validate_mmc(0);
	}
	
	return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

// block from writing boot sectors unless prev asked
static int sdif_write_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (unlikely((sector == E2X_RECOVERY_MBR_OFFSET) && (uint32_t)ctx == E2X_BOOTAREA_LOCK_KEY)) { // change lock mode
		DACR_OFF(
			disable_bootarea_update = nSectors;
		);
		return E2x_BOOTAREA_LOCK_CG_ACK;
	}
	if ((sector < SBLS_END && (sector >= SBLS_START || sector < IDSTOR_START)) && disable_bootarea_update)
		return -1;
	return sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
}

// patch sdif's emmc R/W, clean dirty uids
static int module_load_patched(const SceModuleLoadList* list, int* uids, int count, int unk) {

	// load req modules
	int ret = module_load(list, uids, count, unk);
	if (ret < 0)
		printf("[E2X] E: modload[%d] 0x%X\n", count, ret);

	// skip all unclean uids
	for (int i = 0; i < count; i -= -1) {
		if (uids[i] < 0) {
			printf("[E2X] W: dirty uid[%d]: %s\n", i, (list[i].filename) ? list[i].filename : "NULL");
			uids[i] = 0;
		} else
			printf("[E2X] module loaded [%d]: %s\n", i, (list[i].filename) ? list[i].filename : "NULL");
	}

	// find and patch sdif
	if (count > 13) { // listB
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
						printf("[E2X] E: no sdif obj\n");
					break;
				}
			} else
				printf("[E2X] W: module[%d]: NULL\n", i);
		}
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
	printf("[E2X] get_file [0x%X] @ %s\n", read_size, file_path);
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

static void* load_exe(void* source, char* memblock_name, uint32_t offset, uint32_t size, int flags, int* ret_memblock_id) {
	printf("[E2X] load_exe_%d %s [0x%X] @ 0x%X @ %s\n", flags, memblock_name, size, offset, (flags & E2X_LX_BLK_SAUCE) ? "blkdev" : source);
	if (!size) {
		if (flags & E2X_LX_BLK_SAUCE) // no dynamic size for block devices
			return NULL;
		offset = 0;
		size = get_file((char*)source, NULL, 0, 0);
		if (((int)size < 0) || (offset > size) || !size)
			return NULL;
	}
	
	uint32_t memblock_size = size;
	if (flags & E2X_LX_BLK_SAUCE)
		memblock_size = (memblock_size * SDIF_SECTOR_SIZE);
	memblock_size = ((memblock_size + (0x1000 - 1)) & -0x1000);

	int memblock_id = sceKernelAllocMemBlock(memblock_name, MEMBLOCK_TYPE_RW, memblock_size, NULL);
	void* memblock_va = NULL;
	sceKernelGetMemBlockBase(memblock_id, (void**)&memblock_va);
	if (!memblock_va)
		return NULL;

	int ret;
	if (flags & E2X_LX_BLK_SAUCE)
		ret = read_sector_default_direct((int*)source, offset, size, (int)memblock_va);
	else
		ret = get_file((char*)source, memblock_va, size, offset);
	if (ret < 0) {
		sceKernelFreeMemBlock(memblock_id);
		return NULL;
	}

	if (ret_memblock_id)
		*ret_memblock_id = memblock_id;

	if (flags & E2X_LX_NO_XREMAP)
		return memblock_va;

	sceKernelRemapBlock(memblock_id, MEMBLOCK_TYPE_RX);

	if (flags & E2X_LX_NO_CCACHE)
		return memblock_va;

	clean_dcache(memblock_va, memblock_size);
	flush_icache();

	return memblock_va;
}
// --------------------

/*
	recovery
*/
// recovery from GC-SD
static void recovery(unsigned int ctrl, int dolce) {
	int error = 1, (*rf)(void *kbl_param, unsigned int ctrldata) = NULL;

	printf("[E2X@R] GC-SD recovery mode\n");
	syscon_common_write(1, SYSCON_CMD_SET_GCSD, 2); // enable the GC slot
	boot_args->boot_type_indicator_1 |= 0x40000; // enable sd0 mounting
	clean_dcache((void*)boot_args, 0x100);
	flush_icache();
	setup_emmc(); // reinit main storages

	unsigned char mbr[SDIF_SECTOR_SIZE];
	if (read_sector_default_direct((int*)NSKBL_DEVICE_GCSD_CTX, 0, 1, (int)&mbr) >= 0) {
		if (*(uint32_t*)mbr == 0x796e6f53) { // "Sony" - EMMC dump/SCE formatted GCSD, use its os0 as main os0
			gpio_port_set(0, 7);
			printf("[E2X@R] SCE MBR mode\n");
			init_part((unsigned int*)NSKBL_PARTITION_OS0, 0x110000, (unsigned int*)read_sector_default_direct, (unsigned int*)NSKBL_DEVICE_GCSD_CTX);
			error = 0;
		} else if (*(uint32_t*)(mbr + 0x38) == 0x20363154) { // base+0x36 = 'FA[T16 ]  ' - fat16 partition
			gpio_port_set(0, 7);
			printf("[E2X@R] FAT16 PBR mode\n");
			if (*(uint32_t*)mbr == E2X_MAGIC) { // use as sd0, load recovery from it
				rf = (void*)(load_exe("sd0:" E2X_RECOVERY_FNAME, "recovery", 0, 0, 0, NULL) + 1);
				error = (rf != (void*)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 1;
			} else // use as os0, now there will be os0 and sd0 mounted with the same src
				init_part((unsigned int*)NSKBL_PARTITION_OS0, 0x100000, (unsigned int*)read_sector_default_direct, (unsigned int*)NSKBL_DEVICE_GCSD_CTX);
			error = 0;
		} else if (*(uint32_t*)mbr == E2X_MAGIC) { // enso_ex raw recovery
			gpio_port_set(0, 7);
			printf("[E2X@R] E2XR RAW mode\n");
			RecoveryBlockStruct* recoveryblock = (RecoveryBlockStruct*)mbr;
			if (recoveryblock->flags[0]) // use GCSD os0 as main os0
				init_part((unsigned int*)NSKBL_PARTITION_OS0, 0x110000, (unsigned int*)read_sector_default_direct, (unsigned int*)NSKBL_DEVICE_GCSD_CTX);
			if (recoveryblock->flags[1]) { // use the flashed recovery
				rf = (void*)(load_exe((int*)NSKBL_DEVICE_GCSD_CTX, "recovery", recoveryblock->blkoff, recoveryblock->blksz, 2, NULL) + 1);
				error = (rf != (void*)0x1) ? ((rf(boot_args, ctrl) == 0) ? 0 : 4) : 2;
			}
		} else { // unk magic
			printf("[E2X@R] E: UNK block 0\n");
			error = 2;
		}
	}

	if (!dolce) {
		printf("[E2X@R] status: 0x%X\n", error);
		while (error) {
			syscon_common_read(&ctrl, SYSCON_CMD_GET_DCTRL);
			if ((CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_NOENT) && error == 1) // No SD found
				|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_SDERR) && error == 2) // Incorrect SD magic
				|| (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RETERR) && error == 4)) // Recovery returned !0
				break;
		}
	}
	gpio_port_clear(0, 7);
}
// --------------------

/*
	Custom kernel loader patches
*/
// custom get_hwcfg to give ckldr important offsets
static int get_hwcfg_patched(uint32_t* dst) {
	if (dst[0] == 69) {
		syscon_common_read(&dst[E2X_CHWCFG_CTRL], SYSCON_CMD_GET_DCTRL);
		dst[E2X_CHWCFG_LOAD_EXE] = (uint32_t)load_exe;
		dst[E2X_CHWCFG_KBLPARAM] = (uint32_t)(*sysroot_ctx_ptr)->boot_args;
		dst[E2X_CHWCFG_NSKBL_EXPORTS] = (uint32_t)NSKBL_EXPORTS_ADDR;
		dst[E2X_CHWCFG_GET_FILE] = (uint32_t)get_file;
		return 0;
	} else
		return get_hwcfg(dst);
}

// Run BootMgr and resume psp2bootconfig load with our custom loader
static int load_psp2bootconfig_patched(uint32_t myaddr, int* uids, int count, int osloc, int unk) {
	printf("[E2X] welcome to stage3!\n[E2X] starting kernel load\n");
	
	allow_fselfs();

	*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N) = (uint32_t)get_hwcfg_patched;
	int bootmgr_memblock;
	int (*tcode)(void) = (void*)(load_exe("os0:" E2X_BOOTMGR_NAME, "bootmgr_e2x", 0, 0, 0, &bootmgr_memblock) + 1);
	if (tcode != (void*)0x1) {
		printf("[E2X] run bootmgr\n");
		if (tcode() & 1)
			sceKernelFreeMemBlock(bootmgr_memblock);
	} else
		printf("[E2X] W: no bootmgr\n");

	if (get_file("os0:" E2X_CKLDR_NAME, NULL, 0, 0) > 0)
		myaddr = (uint32_t)E2X_CKLDR_NAME;
	else {
		myaddr = (uint32_t)NSKBL_PSP2BCFG_STRING;
		printf("[E2X] W: no ckldr\n");
	}
	return module_load_direct((SceModuleLoadList*)&myaddr, uids, count, osloc, unk);
}
// --------------------

/*
	enso_ex init stuff
*/
// init os0 for nskbl
static void init_os0(uint32_t mbr_off) {
	// patch MBR offset to emuMBR
	*(uint16_t*)NSKBL_INIT_DEV_SETPARAM_MBR_OFF = 0x2100 | (uint16_t)mbr_off; // movs r1, mbr_off
	clean_dcache((void*)NSKBL_INIT_DEV_SETPARAM_MBR_OFF_CACHER, 0x20);
	flush_icache();

	// set emuMBR offset for the sdif patch
	DACR_OFF(
		fakembr_offset = mbr_off;
	);

	// init os0
	int ret = init_part((unsigned int*)NSKBL_PARTITION_OS0, 0x110000, (unsigned int*)read_sector_default, (unsigned int*)NSKBL_DEVICE_EMMC_CTX);
	printf("[E2X] os0 init[%d]: 0x%08X\n", mbr_off, ret);
	
	// TODO: what do these do? but we need them for some reason
	*(uint32_t*)(NSKBL_PARTITION_OS0 + 0x2C) = 0;
	*(uint32_t*)(NSKBL_PARTITION_OS0 + 0x78) = 0x1A000100;
	*(uint32_t*)(NSKBL_PARTITION_OS0 + 0x84) = 0x1A001000;
	*(uint32_t*)(NSKBL_PARTITION_OS0 + 0x90) = 0x0001002B;
}

// non-standard init
static void custom_init(unsigned int ctrl) {

	printf("[E2X@R] EMMC recovery mode\n");

	// tiny patch from block 4
	if (!CTRL_BUTTON_HELD(ctrl, E2X_BPARAM_NOCUC)) {
		int rconf_memblk = 0;
		void* cc_buf = load_exe((int*)NSKBL_DEVICE_EMMC_CTX, "rconf", E2X_RCONF_OFFSET, 1, E2X_LX_BLK_SAUCE, &rconf_memblk);
		if (cc_buf) {
			void (*ccode)(uint32_t get_info_va, uint32_t init_os0_va, uint32_t load_exe_va) = (void*)(cc_buf + 1);
			printf("[E2X@R] run rconf\n");
			ccode((uint32_t)get_hwcfg_patched, (uint32_t)init_os0, (uint32_t)load_exe); // DO NOT pass fakembr/disable_bootarea_update
			sceKernelFreeMemBlock(rconf_memblk);
		}
	} else {
		// block bootarea writes
		if (CTRL_BUTTON_HELD(ctrl, E2X_BPARAM_LOCKBAREA)) {
			DACR_OFF(
				disable_bootarea_update = 1;
			);
		}

		// init os0
		if (!CTRL_BUTTON_HELD(ctrl, E2X_BPARAM_ADIOS0)) {
			if (CTRL_BUTTON_HELD(ctrl, E2X_BPARAM_CUMBR)) // use recovery emuMBR
				init_os0(E2X_RECOVERY_MBR_OFFSET);
			else
				init_os0(ENSO_EMUMBR_OFFSET);
		}
	}
}

// main
__attribute__((section(".text.start"))) void start(void) {
	gpio_port_clear(0, 7);

	printf("[E2X] welcome to stage2!\n[E2X] patching nskbl\n");

	// ignore module_load error (uids can be unclean now)
	*(uint32_t*)NSKBL_LMODLOAD_CHKRET = 0xbf00bf00;
	clean_dcache((void*)NSKBL_LMODLOAD_CHKRET_CACHER, 0x20);
	flush_icache();

	// use a custom module_load_from_list
	*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_LMODLOAD_N) = (uint32_t)module_load_patched;

	// get pressed buttons
	// we cannot use the info in kbl param - what if syscon is in safe mode?
	unsigned int ctrl;
	syscon_common_read(&ctrl, SYSCON_CMD_GET_DCTRL);
	printf("[E2X] pressed keys: 0x%08X\n", ctrl);

	// change the kernel loader
	if (!CTRL_BUTTON_HELD(ctrl, E2X_IPATCHES_SKIP)) {
		*(uint32_t*)NSKBL_LBOOTM_LPSP2BCFG = 0x47806800; // blx to psp2bootconfig string
		*(uint32_t*)NSKBL_PSP2BCFG_STRING_PTR = (uint32_t)load_psp2bootconfig_patched;
		clean_dcache((void*)NSKBL_LBOOTM_LPSP2BCFG_CACHER, 0x20);
		clean_dcache((void*)NSKBL_PSP2BCFG_STRING_PTR_CACHER, 0x20);
		flush_icache();
	}

	// initialize os0
	if (CTRL_BUTTON_HELD(ctrl, E2X_CHANGE_BPARAM))
		custom_init(ctrl);
	else
		init_os0(ENSO_EMUMBR_OFFSET);

	// Recovery if SELECT held
	if (CTRL_BUTTON_HELD(ctrl, E2X_RECOVERY_RUN))
		recovery(ctrl, 0);
	else if (is_genuine_dolce() && !(CTRL_BUTTON_HELD(ctrl, CTRL_POWER)))
		recovery(ctrl, 1);

	printf("[E2X] return to stage1\n");
}
// --------------------
