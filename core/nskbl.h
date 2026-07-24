/* nsbl.h -- imported data from non-secure bootloader
 *
 * Copyright (C) 2017 molecule, 2018-2022 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef NSBL_HEADER
#define NSBL_HEADER

#include <inttypes.h>

#define NULL ((void *)0)

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

// firmware specific internal structures

typedef struct _kbl_param_s {
  uint16_t version;
  uint16_t size;
  uint32_t sl_ver;
  uint32_t min_sl_ver;
  uint32_t unk_C;
  uint32_t unk_10;
  uint32_t unk_14;
  uint32_t unk_18;
  uint32_t unk_1C;
  struct {
    uint8_t qa[0x10];
    uint8_t nvs[0x10];
    uint8_t dipsw[0x20];
  } flags;
  uint32_t dram_start;
  uint32_t dram_size;
  uint32_t unk_68;
  uint32_t device_mode;
  uint8_t opsid[0x10];
  uint32_t sk_enp_start;
  uint32_t sk_enp_size;
  uint32_t ctxa_sm_start;
  uint32_t ctxa_sm_size;
  uint32_t kprxa_sm_start;
  uint32_t kprxa_sm_size;
  uint32_t srvk_start;
  uint32_t srvk_size;
  uint64_t pscode;
  uint64_t stack_cookie;
  uint8_t session_id[0x10];
  uint32_t wakeup_req;
  uint32_t wakeup_factor;
  uint32_t usb_status;
  uint32_t ctrl;
  uint32_t resume_addr;
  uint32_t hw_cfg;
  uint32_t boot_cause;
  uint32_t unk_DC;
  uint32_t unk_resume;
  uint32_t unk_E4;
  uint8_t hw_cfg_ext[0x10];
  uint32_t sl_rev;
  uint32_t magic;
  uint8_t session_key[0x20];
  uint8_t unused[0xE0];
} __attribute__((packed)) kbl_param_s;

typedef struct SceSysrootContext {
  uint32_t reserved[27];
  kbl_param_s *kbl_param;
} __attribute__((packed)) SceSysrootContext;

typedef struct SceModuleLoadList {
  const char *filename;
} __attribute__((packed)) SceModuleLoadList;

typedef struct SceObject {
  uint32_t field_0;
  void *obj_data;
  char data[];
} __attribute__((packed)) SceObject;

typedef struct SceModuleSegment {
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint16_t p_flags;
  uint16_t p_align_bits;
  void *buf;
  int32_t buf_blkid;
} __attribute__((packed)) SceModuleSegment;

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

#define MEMBLOCK_TYPE_RW 0x1020D006
#define MEMBLOCK_TYPE_RX 0x1020D005 // cached for speed, rember to clean cache(s)
#define SDIF_SECTOR_SIZE 0x200
#define SBLS_START 0x4000 // in sectors
#define SBLS_END 0x8000 // in sectors
#define IDSTOR_START 0x200 // in sectors

// firmware specific function offsets
static void *(*memset)(void *dst, int ch, int sz) = (void*)0x51013C41;
static void *(*memcpy)(void *dst, const void *src, int sz) = (void *)0x51013BC1;
static void *(*memmove)(void *dst, const void *src, int sz) = (void *)0x5102152D;
static void (*clean_dcache)(void *dst, int len) = (void*)0x510146DD;
static void (*flush_icache)() = (void*)0x51014691;
static int (*strncmp)(const char *s1, const char *s2, int len) = (void *)0x51013CA0;
static SceObject *(*get_obj_for_uid)(int uid) = (void *)0x51017785;
static int (*module_load)(const SceModuleLoadList *list, int *uids, int count, int) = (void *)0x51001551;
static int (*module_load_direct)(const SceModuleLoadList *list, int *uids, int count, int osloc, int unk) = (void *)0x5100148d;
static int (*sceKernelAllocMemBlock)(const char *name, int type, int size, SceKernelAllocMemBlockKernelOpt *opt) = (void *)0x51007161;
static int (*sceKernelGetMemBlockBase)(int32_t uid, void **basep) = (void *)0x510057E1;
static int (*sceKernelRemapBlock)(int32_t uid, int type) = (void *)0x51007171;
static int (*sceKernelFreeMemBlock)(int32_t uid) = (void*)0x51007449;

static int (*is_genuine_dolce)(void) = (void*)0x51017321;

static int (*read_sector_sd)(int *part_ctx, uint32_t sector, void *buffer, int nsectors) = (void *)0x5101E879;
static int (*read_sector_mmc_direct)(int *ctx, unsigned int block_offset, void *target_buf, int block_count) = (void *)0x5101c515;
static int (*read_sector_default)(int* ctx, int sector, int nsectors, int buffer) = (void*)0x510010FD; // 0x20 cached
static int (*read_sector_default_direct)(int* pctx, int sector, int nsectors, int buffer) = (void*)0x510010C5;
static int (*setup_emmc)() = (void*)0x5100124D;
static int (*init_part)(unsigned int *partition, unsigned int flags, unsigned int *read_func, unsigned int *master_dev) = (void*)0x5101FF21;

static int (*iof_open)(char *fname, int flags, int mode) = (void *)0x510017D5;
static int (*iof_lseek)(int fd, int fd_hi, uint32_t off, uint32_t off_hi, int mode) = (void *)0x510018a9;
static int (*iof_close)(uint32_t fdlike) = (void *)0x51001901;
static int (*iof_read)(uint32_t fdlike, void* buf, uint32_t sizelike) = (void*)0x510209ed;

static int (*self_auth_header)() = (void*)0x51016ea5;
static int (*self_setup_authseg)() = (void*)0x51016f95;
static int (*self_load_block)() = (void*)0x51016fd1;

static int (*get_hwcfg)(uint32_t* cfgs) = (void*)0x51012a1d;

#ifdef NO_DEBUG_LOGGING
#define printf(...)
#else
static int (*printf)() = (void*)0x51013919;
#endif

// firmware specific patch offsets
static kbl_param_s *kbl_param = (void *)0x51167528;
static SceSysrootContext **sysroot_ctx_ptr = (void *)0x51138A3C;

#define NSKBL_EXPORTS_ADDR 0x5102778c
#define NSKBL_EXPORTS(num) (NSKBL_EXPORTS_ADDR + (num * 4))

#define NSKBL_DEVICE_EMMC_CTX 0x51028010 // for init_part and read_sector_default
#define NSKBL_DEVICE_GCSD_CTX 0x51028018
#define NSKBL_DEVICE_EMMC_TGT_CTX 0x51028014 // for read_sector_target, there is part_ctx @ *this
#define NSKBL_DEVICE_GCSD_TGT_CTX 0x5102801C
#define NSKBL_PARTITION_OS0 0x51167784

#define NSKBL_LOAD_SELF_FUNC 0x51018860
#define NSKBL_LOAD_SELF_COMPRESSED_CHECK 0x5101887a

#define NSKBL_LBOOTM_LPSP2BCFG 0x51001688
#define NSKBL_LBOOTM_LPSP2BCFG_CACHER 0x51001680
#define NSKBL_PSP2BCFG_STRING 0x51023dc0
#define NSKBL_PSP2BCFG_STRING_PTR 0x51023e10
#define NSKBL_PSP2BCFG_STRING_PTR_CACHER 0x51023e00

#define NSKBL_EXPORTS_GET_HWCFG_N 26
#define NSKBL_EXPORTS_LMODLOAD_N 7

#define NSKBL_INIT_DEV_SETPARAM_MBR_OFF 0x510202CE
#define NSKBL_INIT_DEV_SETPARAM_MBR_OFF_CACHER 0x510202C0

#define NSKBL_LMODLOAD_CHKRET 0x510014ee
#define NSKBL_LMODLOAD_CHKRET_CACHER 0x510014e0

#define NSKBL_LMODLOAD_DIR 0x51023de7

#endif
