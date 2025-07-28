#ifndef NSKBL_H
#define NSKBL_H

#include <stdio.h>
#include <string.h>

static int (*nskbl_printf)() = (void *)0x51013919;
#define LOG(fmt, ...) nskbl_printf("[E2X@R] " fmt, ##__VA_ARGS__)

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

static char *(*nskbl_strncpy)(char *dst, const char *src, unsigned int len) = (void *)0x51014611;

#define NSKBL_LBOOTM_LPSP2BCFG 0x51001688
#define NSKBL_LBOOTM_LPSP2BCFG_CACHER 0x51001680
#define NSKBL_PSP2BCFG_STRING 0x51023dc0
#define NSKBL_PSP2BCFG_STRING_PTR 0x51023e10
#define NSKBL_PSP2BCFG_STRING_PTR_CACHER 0x51023e00
static void (*nskbl_clean_dcache)(void *dst, int len) = (void *)0x510146DD;
static void (*nskbl_flush_icache)() = (void *)0x51014691;
static int (*nskbl_module_load_direct)(void *list, int *uids, int count, int osloc, int unk) = (void *)0x5100148d;

#endif
