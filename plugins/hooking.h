/* hooking.h -- defs, structs, funcs related to patching and hooking the kernel modules
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

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
	
typedef struct SceModuleExports {
  uint16_t size;           // size of this structure; 0x20 for Vita 1.x
  uint8_t  lib_version[2]; //
  uint16_t attribute;      // ?
  uint16_t num_functions;  // number of exported functions
  uint16_t num_vars;       // number of exported variables
  uint16_t unk;
  uint32_t num_tls_vars;   // number of exported TLS variables?  <-- pretty sure wrong // yifanlu
  uint32_t lib_nid;        // NID of this specific export list; one PRX can export several names
  char     *lib_name;      // name of the export module
  uint32_t *nid_table;     // array of 32-bit NIDs for the exports, first functions then vars
  void     **entry_table;  // array of pointers to exported functions and then variables
} __attribute__((packed)) SceModuleExports;

#define EI_NIDENT 16
typedef struct Elf32_Ehdr {
  unsigned char e_ident[EI_NIDENT]; /* ident bytes */
  uint16_t  e_type;     /* file type */
  uint16_t  e_machine;    /* target machine */
  uint32_t  e_version;    /* file version */
  uint32_t  e_entry;    /* start address */
  uint32_t e_phoff;    /* phdr file offset */
  uint32_t e_shoff;    /* shdr file offset */
  uint32_t  e_flags;    /* file flags */
  uint16_t  e_ehsize;   /* sizeof ehdr */
  uint16_t  e_phentsize;    /* sizeof phdr */
  uint16_t  e_phnum;    /* number phdrs */
  uint16_t  e_shentsize;    /* sizeof shdr */
  uint16_t  e_shnum;    /* number shdrs */
  uint16_t  e_shstrndx;   /* shdr string index */
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
  uint32_t  p_type;   /* entry type */
  uint32_t p_offset; /* file offset */
  uint32_t  p_vaddr;  /* virtual address */
  uint32_t  p_paddr;  /* physical address */
  uint32_t  p_filesz; /* file size */
  uint32_t  p_memsz;  /* memory size */
  uint32_t  p_flags;  /* entry flags */
  uint32_t  p_align;  /* memory/file alignment */
} __attribute__((packed)) Elf32_Phdr;

typedef struct SceModuleSelfSectionInfo {
  uint64_t offset;
  uint64_t size;
  uint32_t compressed; // 2=compressed
  uint32_t unknown1;
  uint32_t encrypted; // 1=encrypted
  uint32_t unknown2;
} __attribute__((packed)) SceModuleSelfSectionInfo;

typedef struct SceModuleSegment {
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint16_t p_flags;
  uint16_t p_align_bits;
  void *buf;
  int32_t buf_blkid;
} __attribute__((packed)) SceModuleSegment;

typedef struct SceObject {
  uint32_t field_0;
  void *obj_data;
  char data[];
} __attribute__((packed)) SceObject;

typedef struct SceModuleObject {
  struct SceModuleObject *next;
  uint16_t exeflags;
  uint8_t status;
  uint8_t field_7;
  uint32_t min_sysver;
  int32_t modid;
  int32_t user_modid;
  int32_t pid;
  uint16_t modattribute;
  uint16_t modversion;
  uint32_t modid_name;
  SceModuleExports *ent_top_user;
  SceModuleExports *ent_end_user;
  uint32_t stub_start_user;
  uint32_t stub_end_user;
  uint32_t module_nid;
  uint32_t modinfo_field_38;
  uint32_t modinfo_field_3C;
  uint32_t modinfo_field_40;
  uint32_t exidx_start_user;
  uint32_t exidx_end_user;
  uint32_t extab_start_user;
  uint32_t extab_end_user;
  uint16_t num_export_libs;
  uint16_t num_import_libs;
  uint32_t field_54;
  uint32_t field_58;
  uint32_t field_5C;
  uint32_t field_60;
  void *imports;
  const char *path;
  uint32_t total_loadable;
  struct SceModuleSegment segments[3];
  void *type_6FFFFF00_buf;
  uint32_t type_6FFFFF00_bufsz;
  void *module_start;
  void *module_init;
  void *module_stop;
  uint32_t field_C0;
  uint32_t field_C4;
  uint32_t field_C8;
  uint32_t field_CC;
  uint32_t field_D0;
  struct SceObject *prev_loaded;
} __attribute__((packed)) SceModuleObject;

typedef struct SceModuleDecryptContext {
  void *header;
  uint32_t header_len;
  Elf32_Ehdr *elf_ehdr;
  Elf32_Phdr *elf_phdr;
  uint8_t type;
  uint8_t init_completed;
  uint8_t field_12;
  uint8_t field_13;
  SceModuleSelfSectionInfo *section_info;
  void *header_buffer;
  uint32_t sbl_ctx;
  uint32_t field_20;
  uint32_t fd;
  int32_t pid;
  uint32_t max_size;
} __attribute__((packed)) SceModuleDecryptContext;

typedef struct SceKernelAllocMemBlockKernelOpt {
  uint32_t size;
  uint32_t field_4;
  uint32_t attr;
  uint32_t field_C;
  uint32_t paddr;
  uint32_t alignment;
  uint32_t field_18;
  uint32_t field_1C;
  uint32_t mirror_blkid;
  int32_t pid;
  uint32_t field_28;
  uint32_t field_2C;
  uint32_t field_30;
  uint32_t field_34;
  uint32_t field_38;
  uint32_t field_3C;
  uint32_t field_40;
  uint32_t field_44;
  uint32_t field_48;
  uint32_t field_4C;
  uint32_t field_50;
  uint32_t field_54;
} __attribute__((packed)) SceKernelAllocMemBlockKernelOpt;

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