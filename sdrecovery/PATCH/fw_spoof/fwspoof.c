// Make sure that both firmwares in kbl_param and reported by handler are the same

#include <inttypes.h>

#ifdef FW_360
	#include "../../../enso/360/nsbl.h"
#else
	#include "../../../enso/365/nsbl.h"
#endif

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
static void (*sysmem_set_fwver_handler)(uint32_t handler) = NULL;
static int (*sysmem_get_fwver)(void) = NULL;
static int (*sysmem_get_sysroot_part2)(void) = NULL;

static int sysmem_get_fwver_patched(void) {
	return *(uint32_t *)(*(int *)(sysmem_get_sysroot_part2() + 0x6c) + 4);
}

static void sysmem_set_fwver_handler_patched(uint32_t handler) {
	sysmem_set_fwver_handler((uint32_t)sysmem_get_fwver_patched);
	return;
}

void __attribute__((optimize("O0"))) test(PayloadArgsStruct *args) {
	SceObject *obj;
    SceModuleObject *mod;
	const SceModuleLoadList *list = args->list;
	for (int i = 0; i < args->count; i-=-1) {
		if (!list[i].filename)
            continue;
        if (strncmp(list[i].filename, "sysmem.skprx", 12) == 0) { // GCSD support
            obj = get_obj_for_uid(args->uids[i]);
			if (obj != NULL) {
				mod = (SceModuleObject *)&obj->data;
				HOOK_EXPORT(sysmem_set_fwver_handler, 0x2ED7F97A, 0x3276086B);
				HOOK_EXPORT(sysmem_get_fwver, 0x2ED7F97A, 0x67AAB627);
				FIND_EXPORT(sysmem_get_sysroot_part2, 0x3691DA45, 0x3E455842);
			}
		}
    }
	return;
}

__attribute__ ((section (".text.start"))) void start(void *args) {
    test(args);
}