#ifndef NSKBL_H
#define NSKBL_H

#include <stdio.h>
#include <string.h>

static int (*nskbl_printf)() = (void *)0x51013919;

static void *ns_kbl_param = (void *)0x51167528;

typedef struct SceKernelAllocMemBlockKernelOpt {
    uint32_t size;
    uint32_t field_4;
    uint32_t attr;
    uint32_t field_C;
    uint32_t paddr;
    uint32_t alignment;
    uint32_t fields_18[2];
    uint32_t mirror_blkid;
    int32_t pid;
    uint32_t fields_28[12];
} __attribute__((packed)) SceKernelAllocMemBlockKernelOpt;
static int (*sceKernelAllocMemBlock)(const char *name, int type, int size, SceKernelAllocMemBlockKernelOpt *opt) = (void *)0x51007161;
static int (*sceKernelGetMemBlockBase)(int32_t uid, void **basep) = (void *)0x510057E1;
static int (*sceKernelRemapBlock)(int32_t uid, int type) = (void *)0x51007171;
static int (*sceKernelFreeMemBlock)(int32_t uid) = (void *)0x51007449;
#define MEMBLOCK_TYPE_RW 0x1020D006
#define MEMBLOCK_TYPE_RX 0x1020D005  // cached for speed, rember to clean cache(s)

static char *(*nskbl_strncpy)(char *dst, const char *src, unsigned int len) = (void *)0x51014611;
static int (*nskbl_snprintf)(char *buf, unsigned int size, const char *fmt, ...) = (void *)0x510145c9;

#define NSKBL_LBOOTM_LPSP2BCFG 0x51001688
#define NSKBL_LBOOTM_LPSP2BCFG_CACHER 0x51001680
#define NSKBL_PSP2BCFG_STRING 0x51023dc0
#define NSKBL_PSP2BCFG_STRING_PTR 0x51023e10
#define NSKBL_PSP2BCFG_STRING_PTR_CACHER 0x51023e00
static void (*nskbl_clean_dcache)(void *dst, int len) = (void *)0x510146DD;
static void (*nskbl_flush_icache)() = (void *)0x51014691;
static int (*nskbl_module_load_direct)(void *list, int *uids, int count, int osloc, int unk) = (void *)0x5100148d;

#define NSKBL_DEVICE_EMMC_CTX 0x51028010  // for init_part and read_sector_default
#define NSKBL_DEVICE_GCSD_CTX 0x51028018
#define NSKBL_DEVICE_EMMC_TGT_CTX 0x51028014  // for read_sector_target, there is part_ctx @ *this
#define NSKBL_DEVICE_GCSD_TGT_CTX 0x5102801C
#define NSKBL_PARTITION_OS0 0x51167784
#define NSKBL_PARTITION_SD0 0x51167728
static int (*read_sector_sd)(int *part_ctx, uint32_t sector, void *buffer, int nsectors) = (void *)0x5101E879;
static int (*read_sector_mmc_direct)(int *ctx, unsigned int block_offset, void *target_buf, int block_count) = (void *)0x5101c515;
static int (*lsdif_mmc_verify_args)(int *ctx, unsigned int block_offset, int block_count) = (void *)0x5101c23d;
static int (*lsdif_mmc_prep_ctx)() = (void *)0x5101c091;
static int (*lsysclib_concat_unk)() = (void *)0x510221fc;
static int (*lsdif_mmc_prepare_args)() = (void *)0x5101bbf9;
static int (*lsdif_mmc_write_args)() = (void *)0x5101c0a5;
static int (*lsdif_ctrl_apply_cmd)() = (void *)0x5101bf65;

static int (*nskbl_init_sd)(unsigned int *in_master_dev, int *some_ret) = (void *)0x5101da29;
static int (*nskbl_init_part)(unsigned int *partition, unsigned int flags, unsigned int *read_func, unsigned int *master_dev) = (void *)0x5101FF21;
static int (*nskbl_switch_read_dev)(int *ctx, int sector, int nSectors, int buffer) = (void *)0x510010c5;

#define NSKBL_SETUP_EMMC_INIT_OS0_CALL 0x510012f6
#define NSKBL_SETUP_EMMC_INIT_OS0_CALL_CACHER 0x510012f0
static int (*nskbl_setup_emmc)() = (void *)0x5100124D;

#endif
