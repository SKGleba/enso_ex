// Redirect sdif emmc read/writes to GCSD

#include <inttypes.h>

#ifdef FW_360
	#include "../../../enso/360/nsbl.h"
#else
	#include "../../../enso/365/nsbl.h"
#endif

#include "../../../enso/bm_compat.h"
#include "../../../enso/ex_defs.h"

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
	
#define INSTALL_RET_THUMB(addr, ret)   \
do {                                   \
    unsigned *target;                  \
    target = (unsigned*)(addr);        \
    *target = 0x47702000 | (ret); /* movs r0, #ret; bx lr */ \
} while (0)

#define SDSTOR_PROC_CHKO (0x2498)
#define SDSTOR_PROC_CHKT (0x2940)
static const char sdstor_suppl_patch[4] = {0x01, 0x20, 0x00, 0xBF};
static const char movsr01[2] = {0x01, 0x20};

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

// sdif funcs
static int (*sdif_read_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_read_sector_sd)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_mmc)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdif_write_sector_sd)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void *(*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;
static void *(*get_sd_context_part_validate_sd)(int sd_ctx_index) = NULL;

static int sdif_read_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (get_sd_context_part_validate_mmc(0) == ctx) {
		int ret = sdif_read_sector_sd(get_sd_context_part_validate_sd(1), sector, buffer, nSectors);
		if (ret < 0)
			ret = sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
		return ret;
	}
	return sdif_read_sector_mmc(ctx, sector, buffer, nSectors);
}

static int sdif_write_sector_mmc_patched(void* ctx, int sector, char* buffer, int nSectors) {
	if (get_sd_context_part_validate_mmc(0) == ctx) {
		int ret = sdif_write_sector_sd(get_sd_context_part_validate_sd(1), sector, buffer, nSectors);
		if (ret < 0)
			ret = sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
		return ret;
	}
	return sdif_write_sector_mmc(ctx, sector, buffer, nSectors);
}

void __attribute__((optimize("O0"))) test(PayloadArgsStruct *args) {
	SceObject *obj;
    SceModuleObject *mod;
	const SceModuleLoadList *list = args->list;
	for (int i = 0; i < args->count; i-=-1) {
		if (!list[i].filename)
            continue;
        if (CTRL_BUTTON_HELD(args->ctrldata, CTRL_START) && strncmp(list[i].filename, "sdif.skprx", 10) == 0) { // redirect patches
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				HOOK_EXPORT(sdif_read_sector_mmc, 0x96D306FA, 0x6F8D529B);
				FIND_EXPORT(sdif_read_sector_sd, 0x96D306FA, 0xB9593652);
				FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
				FIND_EXPORT(get_sd_context_part_validate_sd, 0x96D306FA, 0xB9EA5B1E);
				FIND_EXPORT(sdif_write_sector_sd, 0x96D306FA, 0xE0781171);
				HOOK_EXPORT(sdif_write_sector_mmc, 0x96D306FA, 0x175543D2);
			}
		} else if (CTRL_BUTTON_HELD(args->ctrldata, CTRL_START) && strncmp(list[i].filename, "sdstor.skprx", 12) == 0) { // support for nauthed GC
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				DACR_OFF(
					memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKO, sdstor_suppl_patch, sizeof(sdstor_suppl_patch));
					memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKT, sdstor_suppl_patch, sizeof(sdstor_suppl_patch));
				);
			}
		} else if (CTRL_BUTTON_HELD(args->ctrldata, CTRL_START) && strncmp(list[i].filename, "sysmem.skprx", 12) == 0) { // GCSD support
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				DACR_OFF(
					memcpy(mod->segments[0].buf + 0x21610, movsr01, sizeof(movsr01));
				);
			}
		}
    }
	return;
}

__attribute__ ((section (".text.start"))) void start(void *args) {
    test(args);
}