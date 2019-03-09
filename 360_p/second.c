/* second.c -- bootloader patches
 *
 * Copyright (C) 2017 molecule
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>
#include <psp2kern/display.h>
#include "nsbl.h"
#include "logo.h"

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
		
#define LOG_FILE(path, create, ...) \
	do { \
		char buffer[256]; \
		rsnprintfr(buffer, sizeof(buffer), ##__VA_ARGS__); \
		logg(buffer, rstrlenr(buffer), path, create); \
} while (0)
	
#define LOG(...) \
	do { \
		char buffer[256]; \
		rsnprintfr(buffer, sizeof(buffer), ##__VA_ARGS__); \
		logg(buffer, rstrlenr(buffer), "ur0:tai/log.ex", 2); \
} while (0)
	
typedef enum SceKernelMemBlockType {
	SCE_KERNEL_MEMBLOCK_TYPE_SHARED_RX                = 0x0390D050,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW            = 0x09408060,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE          = 0x0C208060,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_RX                  = 0x0C20D050,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_RW                  = 0x0C20D060,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW     = 0x0C80D060,
	SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW  = 0x0D808060,
	SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RX                = 0x1020D005,
	SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW                = 0x1020D006,
	SCE_KERNEL_MEMBLOCK_TYPE_RW_UNK0                  = 0x6020D006
} SceKernelMemBlockType;

typedef enum SceIoMode {
	SCE_O_RDONLY    = 0x0001,                         //!< Read-only
	SCE_O_WRONLY    = 0x0002,                         //!< Write-only
	SCE_O_RDWR      = (SCE_O_RDONLY | SCE_O_WRONLY),  //!< Read/Write
	SCE_O_NBLOCK    = 0x0004,                         //!< Non blocking
	SCE_O_DIROPEN   = 0x0008,                         //!< Internal use for ::ksceIoDopen
	SCE_O_RDLOCK    = 0x0010,                         //!< Read locked (non-shared)
	SCE_O_WRLOCK    = 0x0020,                         //!< Write locked (non-shared)
	SCE_O_APPEND    = 0x0100,                         //!< Append
	SCE_O_CREAT     = 0x0200,                         //!< Create
	SCE_O_TRUNC     = 0x0400,                         //!< Truncate
	SCE_O_EXCL      = 0x0800,                         //!< Exclusive create
	SCE_O_SCAN      = 0x1000,                         //!< Scan type
	SCE_O_RCOM      = 0x2000,                         //!< Remote command entry
	SCE_O_NOBUF     = 0x4000,                         //!< Number device buffer
	SCE_O_NOWAIT    = 0x8000,                         //!< Asynchronous I/O
	SCE_O_FDEXCL    = 0x01000000,                     //!< Exclusive access
	SCE_O_PWLOCK    = 0x02000000,                     //!< Power control lock
	SCE_O_FGAMEDATA = 0x40000000                      //!< Gamedata access
} SceIoMode;

typedef enum SceIoSeekMode {
	SCE_SEEK_SET,   //!< Starts from the begin of the file
	SCE_SEEK_CUR,   //!< Starts from current position
	SCE_SEEK_END    //!< Starts from the end of the file
} SceIoSeekMode;

 typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);
 
 typedef struct SceKernelThreadOptParam {
     SceSize   size;
     SceUInt32       attr;
 } SceKernelThreadOptParam;

 typedef struct SceIoStat {
     SceMode st_mode;             
     unsigned int st_attr;        
     SceOff st_size;              
     SceDateTime st_ctime;        
     SceDateTime st_atime;        
     SceDateTime st_mtime;        
     unsigned int st_private[6];  
 } SceIoStat;

// sdstor restore globals
static int (*sdstor_read_sector_async)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static int (*sdstor_read_sector)(void* ctx, int sector, char* buffer, int nSectors) = NULL;
static void *(*get_sd_context_part_validate_mmc)(int sd_ctx_index) = NULL;

// debug globals
#ifdef DEBUG
static int (*set_crash_flag)(int) = NULL;
#endif

// sigpatch globals
static int g_sigpatch_disabled = 0;
static int g_homebrew_decrypt = 0;
static int (*sbl_parse_header)(uint32_t ctx, const void *header, int len, void *args) = NULL;
static int (*sbl_set_up_buffer)(uint32_t ctx, int segidx) = NULL;
static int (*sbl_decrypt)(uint32_t ctx, void *buf, int sz) = NULL;

// modmgr
static int (*KLoadStartModule)(const char* modpath, int arg1, void* s2, int arg3, int arg4, int arg5) = NULL;

// useful
static int (*rstrncmpr)(const char *s1, const char *s2, int len) = NULL;
static int (*rsnprintfr)(char *s1, int a1, const char *s2, ...) = NULL;
static int (*rstrlenr)(const char *s1) = NULL;

// iofilemgr functs
static int (*rIoOpen)(const char *s1, int arg1, int arg2) = NULL;
static int (*rIoLseek)(unsigned int fd, unsigned long offset_high, unsigned long offset_low, uint32_t *result, unsigned int whence) = NULL;
static int (*rIoRead)(int arg0, void *data, int arg2) = NULL;
static int (*rIoWrite)(int arg0, void *data, int arg2) = NULL;
static int (*rIoClose)(int arg0) = NULL;
static int (*rIoMount)(int a1, const char *s1, int a3, int a4, int a5, int a6) = NULL;
static int (*rIoUmount)(int a1, int a2, int a3, int a4) = NULL;
static int (*rIoRemove)(const char *s1) = NULL;
static int (*rIoRename)(const char *s1, const char *s2) = NULL;
static int (*rIoGetstat)(const char *file, SceIoStat *stat) = NULL;
//static int (*rVfsNodeInitializePartition)(int *node, int *new_node_p, void *opt, int flags) = NULL;
static int (*rIoMkdir)(const char *dir, SceMode mode) = NULL;

//Sysmem func
static int (*rKernelAllocMemBlock)(const char *name, SceKernelMemBlockType type, int size, SceKernelAllocMemBlockKernelOpt *optp) = NULL;
static int (*rKernelGetMemBlockBase)(int uid, void **basep) = NULL;
static int (*rKernelFreeMemBlock)(int uid) = NULL;
static int (*chkdipsw)(int arg0) = NULL;
static int (*setdipsw)(int arg0) = NULL;
static int (*clrdipsw)(int arg0) = NULL;
static int (*rGzipDecompress)(void *dst, uint32_t dst_size, const void *src, uint32_t *crc32) = NULL;	

//cpu
static int (*rKernelCpuDcacheAndL2WritebackInvalidateRange)(const void *ptr, size_t len) = NULL;

//display
static int (*rDisplaySetFrameBuf)(const SceDisplayFrameBuf *pParam, int sync) = NULL;
static int (*rDisplayWaitVblankStart)(void) = NULL;

//threadmgr
static int (*rKernelStartThread)(SceUID thid, SceSize arglen, void *argp) = NULL;;
static int (*rKernelExitDeleteThread)(int status) = NULL;
static int (*rKernelCreateThread)(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMas, const SceKernelThreadOptParam *option) = NULL;

// sysstate final function
static void __attribute__((noreturn)) (*sysstate_final)(void) = NULL;

// utility functions

#if 0
static int hex_dump(const char *addr, unsigned int size)
{
    unsigned int i;
    for (i = 0; i < (size >> 4); i++)
    {
        printf("0x%08X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", addr, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7], addr[8], addr[9], addr[10], addr[11], addr[12], addr[13], addr[14], addr[15]);
        addr += 0x10;
    }
    return 0;
}
#endif

static int logg(void *buffer, int length, const char* logloc, int create)
{
	int fd;
	if (create == 0) {
		fd = rIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND, 6);
	} else if (create == 1) {
		fd = rIoOpen(logloc, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	} else if (create == 2) {
		fd = rIoOpen(logloc, SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 6);
	}
	if (fd < 0)
		return 0;

	rIoWrite(fd, buffer, length);
	rIoClose(fd);
	return 1;
}

static int ex(const char* filloc){
  int fd;
  fd = rIoOpen(filloc, SCE_O_RDONLY, 0777);
  if (fd < 0) return 0;
  rIoClose(fd); return 1;
}

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

static int checkflag(int flagno) { // 9iq
	int val = 0;
	const char* cfg = "1000-CFG"; // LOGO, CDEV, SUP, DUP - MAGIC
	char fbuf[4];
	memcpy((char *)fbuf, cfg, 4);
	val = fbuf[flagno] - 0x30;
	return val;
}

// sdif patches for MBR redirection

static int sdstor_read_sector_patched(void* ctx, int sector, char* buffer, int nSectors) {
    int ret;
#ifndef NO_MBR_REDIRECT
    if (unlikely(sector == 0 && nSectors > 0)) {
        printf("read sector 0 for %d at context 0x%08X\n", nSectors, ctx);
        if (get_sd_context_part_validate_mmc(0) == ctx) {
            printf("patching sector 0 read to sector 1\n");
            ret = sdstor_read_sector(ctx, 1, buffer, 1);
            if (ret >= 0 && nSectors > 1) {
                ret = sdstor_read_sector(ctx, 1, buffer + 0x200, nSectors-1);
            }
            return ret;
        }
    }
#endif

    return sdstor_read_sector(ctx, sector, buffer, nSectors);
}

static int sdstor_read_sector_async_patched(void* ctx, int sector, char* buffer, int nSectors) {
    int ret;
#ifndef NO_MBR_REDIRECT
    if (unlikely(sector == 0 && nSectors > 0)) {
        printf("read sector async 0 for %d at context 0x%08X\n", nSectors, ctx);
        if (get_sd_context_part_validate_mmc(0) == ctx) {
            printf("patching sector 0 read to sector 1\n");
            ret = sdstor_read_sector_async(ctx, 1, buffer, 1);
            if (ret >= 0 && nSectors > 1) {
                ret = sdstor_read_sector_async(ctx, 1, buffer + 0x200, nSectors-1);
            }
            return ret;
        }
    }
#endif

    return sdstor_read_sector_async(ctx, sector, buffer, nSectors);
}

static int LoadBootlogoSingle(int mode) {
	int ret = 1;
	SceIoStat stat;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	if (mode == 1) {
		fd = rIoOpen("ur0:tai/boot_splash.img", SCE_O_RDONLY, 0);
		rIoGetstat("ur0:tai/boot_splash.img", &stat);
	} else {
		fd = rIoOpen("sa0:eex/boot_splash.img", SCE_O_RDONLY, 0);
		rIoGetstat("sa0:eex/boot_splash.img", &stat);
	}
	uid = rKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	rKernelGetMemBlockBase(uid, (void**)&fb_addr);
	if (stat.st_size < 0x1FE000) {
		yid = rKernelAllocMemBlock("r", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
		rKernelGetMemBlockBase(yid, (void**)&gz_addr);
		rIoRead(fd, (void *)gz_addr, stat.st_size);
		rGzipDecompress((void *)fb_addr, 0x1FE000, (void *)gz_addr, NULL);
		rKernelFreeMemBlock(yid);
	} else {
		rIoRead(fd, (void *)fb_addr, 0x1FE000);
	}
	rIoClose(fd);
	rKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
	fb.size        = sizeof(fb);
	fb.base        = fb_addr;
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;
	rDisplaySetFrameBuf(&fb, 1);
	rDisplayWaitVblankStart();
	rKernelFreeMemBlock(uid);
	ret = 0;
	return ret;
}

static int banimthread(SceSize args, void *argp) {
	uint32_t cur = 0, max = 0;
	SceKernelAllocMemBlockKernelOpt optp;
	SceDisplayFrameBuf fb;
	void *fb_addr = NULL, *gz_addr = NULL;
	int uid, yid, fd;
	uint32_t csz = 0;
	char rmax[4], rsz[4], flags[4];
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1C000000;
	fb.size        = sizeof(fb);		
	fb.pitch       = 960;
	fb.pixelformat = 0;
	fb.width       = 960;
	fb.height      = 544;
	fd = rIoOpen("ur0:tai/boot_animation.img", SCE_O_RDONLY, 0);
	if (fd < 0) {
		clrdipsw(0);
		rKernelExitDeleteThread(0);
		return 1;
	}
	rIoRead(fd, rmax, sizeof(rmax));
	max = *(uint32_t *)rmax;
	rIoRead(fd, flags, sizeof(flags));
	uid = rKernelAllocMemBlock("SceDisplay", 0x6020D006, 0x200000, &optp);
	rKernelGetMemBlockBase(uid, (void**)&fb_addr);
	if (flags[1] == 1) {
		yid = rKernelAllocMemBlock("h", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x200000, NULL);
		rKernelGetMemBlockBase(yid, (void**)&gz_addr);
	}
	while (chkdipsw(0) == 1) {
		if (cur > max && flags[0] > 0) {
			rIoLseek(fd, 0, 0, 0, 0);
			rIoRead(fd, rmax, sizeof(rmax));
			max = *(uint32_t *)rmax;
			rIoRead(fd, flags, sizeof(flags));
			cur = 0;
		} else if (cur > max && flags[0] == 0) {
			clrdipsw(0);
			rKernelFreeMemBlock(uid);
			rKernelFreeMemBlock(yid);
			rIoClose(fd);
			rKernelExitDeleteThread(0);
			return 1;
		}
		rIoRead(fd, rsz, sizeof(rsz));
		csz = *(uint32_t *)rsz;
		if (flags[1] == 1) {
			rIoRead(fd, gz_addr, csz);
			rGzipDecompress((void *)fb_addr, 0x1FE000, (void *)gz_addr, NULL);
		} else {
			rIoRead(fd, fb_addr, csz);
		}
		
		rKernelCpuDcacheAndL2WritebackInvalidateRange(fb_addr, 0x1FE000);
		fb.base = fb_addr;
		rDisplaySetFrameBuf(&fb, 1);
		rDisplayWaitVblankStart();
		cur++;
	}
	rKernelFreeMemBlock(uid);
	if (flags[1] == 1) rKernelFreeMemBlock(yid);
	rIoClose(fd);
	rKernelExitDeleteThread(0);
	return 1;
}

static int KLoadStartModule_Anim(const char* modpath, int arg1, void* s2, int arg3, int arg4, int arg5) {
 int ret = 0;
 ret = KLoadStartModule(modpath, arg1, s2, arg3, arg4, arg5);
 if (rstrncmpr(modpath, "os0:kd/clockgen.skprx", 21) == 0 && ret >= 0) {
	if (ex("ur0:tai/boot_splash.img") == 1) {
		LoadBootlogoSingle(1);
	} else if (ex("ur0:tai/boot_animation.img") == 1) {
		SceUID athid = rKernelCreateThread("b", banimthread, 0x00, 0x1000, 0, 0, 0);
		setdipsw(0);
		rKernelStartThread(athid, 0, NULL);
	} else if (ex("sa0:eex/boot_splash.img") == 1) {
		LoadBootlogoSingle(0);
	}
	rIoMount(0x800, NULL, 0, 0, 0, 0);
	if (ex("ur0:tai/boot_config.txt") == 0 && ex("ur0:tai/config.txt") == 0) {
		rIoMkdir("ur0:tai", 0777);
		LOG_FILE("ur0:tai/config.txt", 1, "# enso_ex temp config\n*main\nsa0:eex/henkaku.suprx\n*NPXS10015\nsa0:eex/henkaku.suprx\n*NPXS10016\nsa0:eex/henkaku.suprx\n");
		rIoRemove("ux0:id.dat");
	}
 } else if (rstrncmpr(modpath, "ur0:tai/henkaku.skprx", 20) == 0 && ret >= 0) {
		int rx = KLoadStartModule("sa0:eex/mmt.skprx", 0, NULL, 0, 0, 0);
		if (rx < 0) rIoMount(0x800, NULL, 0, 0, 0, 0);
		clrdipsw(0);
 } else if (rstrncmpr(modpath, "sa0:eex/henkaku.skprx", 20) == 0 && ret >= 0) {
		int rx = KLoadStartModule("sa0:eex/mmt.skprx", 0, NULL, 0, 0, 0);
		if (rx < 0) rIoMount(0x800, NULL, 0, 0, 0, 0);
		clrdipsw(0);
 }
 return ret;
}

static int KLoadStartModule_Def(const char* modpath, int arg1, void* s2, int arg3, int arg4, int arg5) {
 int ret = 0;
 ret = KLoadStartModule(modpath, arg1, s2, arg3, arg4, arg5);
 if (rstrncmpr(modpath, "os0:kd/clockgen.skprx", 21) == 0 && ret >= 0) {
	rIoMount(0x800, NULL, 0, 0, 0, 0);
	if (ex("ur0:tai/boot_config.txt") == 0 && ex("ur0:tai/config.txt") == 0) {
		rIoMkdir("ur0:tai", 0777);
		LOG_FILE("ur0:tai/config.txt", 1, "# enso_ex temp config\n*main\nsa0:eex/henkaku.suprx\n*NPXS10015\nsa0:eex/henkaku.suprx\n*NPXS10016\nsa0:eex/henkaku.suprx\n");
		rIoRemove("ux0:id.dat");
	}
 } else if (rstrncmpr(modpath, "ur0:tai/henkaku.skprx", 20) == 0 && ret >= 0) {
		int rx = KLoadStartModule("sa0:eex/mmt.skprx", 0, NULL, 0, 0, 0);
		if (rx < 0) rIoMount(0x800, NULL, 0, 0, 0, 0);
		clrdipsw(0);
 } else if (rstrncmpr(modpath, "sa0:eex/henkaku.skprx", 20) == 0 && ret >= 0) {
		int rx = KLoadStartModule("sa0:eex/mmt.skprx", 0, NULL, 0, 0, 0);
		if (rx < 0) rIoMount(0x800, NULL, 0, 0, 0, 0);
		clrdipsw(0);
 }
 return ret;
}

static int rIoOpen_sup(char *s1, int arg1, int arg2) {
 int ret = 0;
	if (rstrncmpr(s1, "tty0:", 5) == 0) s1 = "ur0:tai/tty0_upp.log";
	if (rstrncmpr(s1, "tty1:", 5) == 0) s1 = "ur0:tai/tty1_upp.log";
	if (rstrncmpr(s1, "sdstor0:int-lp-ign-vsh", 22) == 0){
		rIoMount(0x300, NULL, 0x2, 0, 0, 0);
		if (ex("vs0:/vsh/shell/shell.self") == 1 && ex("ur0:tai/supp.x") == 1) {
			s1 = "ur0:tai/IM.BI";
			rIoRemove("ud0:PSP2UPDATE/PSP2UPDAT.PUP");
		}
		rIoUmount(0x300, 0x1, 0, 0);
	}
	ret = rIoOpen(s1, arg1, arg2);
 return ret;
}

static int rIoOpen_dup(char *s1, int arg1, int arg2) {
 int ret = 0;
 rIoRemove("ud0:PSP2UPDATE/PSP2UPDAT.PUP");
 ret = rIoOpen(s1, arg1, arg2);
 return ret;
}

/*
static int rVfsNodeInitializePartition_patched(int *node, int *new_node_p, void *opt, int flags) { // Suspend fix
  int res = rVfsNodeInitializePartition(node, new_node_p, opt, flags);

  if (res == 0 && new_node_p) {
    int *new_node = (int *)*new_node_p;
    int *mount = (int *)new_node[19];
    mount[20] &= ~0x10000;
  }

  return res;
}
*/

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
    printf("after kernel load! disabling temporary sigpatches\n");

    DACR_OFF(
        g_sigpatch_disabled = 1;
    );

    sysstate_final();
}

// main function to hook stuff

#define HOOK_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    printf(#name ": 0x%08X\n", *func);                      \
    DACR_OFF(                                               \
        name = *func;                                       \
        *func = name ## _patched;                           \
    );                                                      \
} while (0)
#define HOOK_EXPORT_CUS(name, lib_nid, func_nid, hname) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    printf(#name ": 0x%08X\n", *func);                      \
    DACR_OFF(                                               \
        name = *func;                                       \
        *func = hname;                           \
    );                                                      \
} while (0)
#define FIND_EXPORT(name, lib_nid, func_nid) do {           \
    void **func = get_export_func(mod, lib_nid, func_nid);  \
    printf(#name ": 0x%08X\n", *func);                      \
    DACR_OFF(                                               \
        name = *func;                                       \
    );                                                      \
} while (0)
static int module_load_patched(const SceModuleLoadList *list, int *uids, int count, int unk) {
    int ret;
    SceObject *obj;
    SceModuleObject *mod;
    int skip;
    int sysmem_idx = -1, display_idx = -1, sdif_idx = -1, authmgr_idx = -1, sysstate_idx = -1, modmgr_idx = -1, sdstor_idx = -1, iofilemgr_idx = -1, threadmgr_idx = -1;

    skip = skip_patches();
    for (int i = 0; i < count; i++) {
        if (!list[i].filename) {
            continue; // wtf sony why don't you sanitize input
        }
        printf("before start %s\n", list[i].filename);
        if (!skip && strncmp(list[i].filename, "display.skprx", 13) == 0) {
            display_idx = i;
        } else if (strncmp(list[i].filename, "sdif.skprx", 10) == 0) {
            sdif_idx = i; // never skip MBR redirection patches
        } else if (!skip && strncmp(list[i].filename, "authmgr.skprx", 13) == 0) {
            authmgr_idx = i;
        } else if (!skip && strncmp(list[i].filename, "sysstatemgr.skprx", 17) == 0) {
            sysstate_idx = i;
        } else if (strncmp(list[i].filename, "iofilemgr.skprx", 15) == 0) {
            iofilemgr_idx = i;
        } else if (strncmp(list[i].filename, "sysmem.skprx", 12) == 0) {
            sysmem_idx = i;
        } else if (!skip && strncmp(list[i].filename, "threadmgr.skprx", 15) == 0) {
            threadmgr_idx = i;
        } else if (!skip && strncmp(list[i].filename, "modulemgr.skprx", 15) == 0) {
            modmgr_idx = i;
        } else if (!skip && strncmp(list[i].filename, "sdstor.skprx", 12) == 0) {
            sdstor_idx = i;
        }

    }
    ret = module_load(list, uids, count, unk);

    // get sysmem functions
    if (sysmem_idx >= 0) {
        obj = get_obj_for_uid(uids[sysmem_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
//				FIND_EXPORT(set_crash_flag, 0x13D793B7, 0xA465A31A);
				FIND_EXPORT(rstrncmpr, 0x7EE45391, 0x12CEE649);
				FIND_EXPORT(rstrlenr, 0x7EE45391, 0xCFC6A9AC);
				FIND_EXPORT(rsnprintfr, 0x7EE45391, 0xAE7A8981);
				FIND_EXPORT(rKernelAllocMemBlock, 0x6F25E18A, 0xC94850C9);
				FIND_EXPORT(rKernelGetMemBlockBase, 0x6F25E18A, 0xA841EDDA);
				FIND_EXPORT(rKernelFreeMemBlock, 0x6F25E18A, 0x009E1C61);
				FIND_EXPORT(rKernelCpuDcacheAndL2WritebackInvalidateRange, 0x40ECDB0E, 0x364E68A4);
				FIND_EXPORT(setdipsw, 0xC9E26388, 0x82E45FBF);
				FIND_EXPORT(chkdipsw, 0xC9E26388, 0xA98FC2FD);
				FIND_EXPORT(clrdipsw, 0xC9E26388, 0xF1F3E9FE);
				FIND_EXPORT(rGzipDecompress, 0x496AD8B4, 0x367EE3DF);
//				FIND_EXPORT(printf, 0x88758561, 0x391B74B7);

        } else {
            printf("module data invalid for sysmem.skprx!\n");
        }
    }
    // patch logo
    if (display_idx >= 0 && checkflag(0) == 1) {
        obj = get_obj_for_uid(uids[display_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            printf("logo at offset: %x\n", mod->segments[0].buf + SCEDISPLAY_LOGO_OFFSET);
            DACR_OFF(
                memcpy(mod->segments[0].buf + SCEDISPLAY_LOGO_OFFSET, logo_data, logo_len);
            );
			FIND_EXPORT(rDisplaySetFrameBuf, 0x9FED47AC, 0x289D82FE);
			FIND_EXPORT(rDisplayWaitVblankStart, 0x9FED47AC, 0x984C27E7);
            // no cache flush needed because this is just data
        } else {
            printf("module data invalid for display.skprx!\n");
        }
    }
	
	// patch sdstor for gcd shit
    if (sdstor_idx >= 0 && checkflag(1) == 1) {
        obj = get_obj_for_uid(uids[sdstor_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            DACR_OFF(
                memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKO, zeroCallOnePatch, sizeof(zeroCallOnePatch));
                memcpy(mod->segments[0].buf + SDSTOR_PROC_CHKT, zeroCallOnePatch, sizeof(zeroCallOnePatch));
            );
        } else {
            printf("module data invalid for sdstor.skprx!\n");
        }
    }
	
	// patch iofilemgr for redir shit
    if (iofilemgr_idx >= 0) {
        obj = get_obj_for_uid(uids[iofilemgr_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
			if (!skip) {
				DACR_OFF(
					if (checkflag(1) == 1) { // sd2vita as ux0
						memcpy(mod->segments[0].buf + IOF_XMC0_STRING, sd0_blkn, sizeof(sd0_blkn)); // if mcs inserted - "sdstor0:ext-lp-act-entire"
						memcpy(mod->segments[0].buf + IOF_IMC_STRING, sd0_blkn_s, sizeof(sd0_blkn_s)); // if mcs removed - "sdstor0:gcd-lp-act-entire"
					}
				);
			}
			FIND_EXPORT(rIoClose, 0x40FD29C7, 0xF99DD8A3);
			FIND_EXPORT(rIoLseek, 0x40FD29C7, 0x62090481);
			FIND_EXPORT(rIoMount, 0x40FD29C7, 0xD070BC48);
			FIND_EXPORT(rIoRead, 0x40FD29C7, 0xE17EFC03);
			FIND_EXPORT(rIoRemove, 0x40FD29C7, 0x0D7BB3E1);
			FIND_EXPORT(rIoRename, 0x40FD29C7, 0xDC0C4997);
			FIND_EXPORT(rIoUmount, 0x40FD29C7, 0x20574100);
			FIND_EXPORT(rIoWrite, 0x40FD29C7, 0x21EE91F0);
			FIND_EXPORT(rIoGetstat, 0x40FD29C7, 0x75C96D25);
			//if (!skip) HOOK_EXPORT(rVfsNodeInitializePartition, 0x40FD29C7, 0xA5A6A55C);
			FIND_EXPORT(rIoMkdir, 0x40FD29C7, 0x7F710B25);
			
			if (is_update_mode() == 1 && is_cable() == 0 && checkflag(2) == 1 && checkflag(3) == 0) {
				HOOK_EXPORT_CUS(rIoOpen, 0x40FD29C7, 0x75192972, rIoOpen_sup);
			} else if (is_update_mode() == 1 && is_cable() == 0 && checkflag(2) == 0 && checkflag(3) == 1) {
				HOOK_EXPORT_CUS(rIoOpen, 0x40FD29C7, 0x75192972, rIoOpen_dup);
			} else {
				FIND_EXPORT(rIoOpen, 0x40FD29C7, 0x75192972);
			} 
        } else {
            printf("module data invalid for iofilemgr.skprx!\n");
        }
    }
	
	// patch threadmgr
    if (threadmgr_idx >= 0) {
        obj = get_obj_for_uid(uids[threadmgr_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            FIND_EXPORT(rKernelCreateThread, 0xE2C40624, 0xC6674E7D);
			FIND_EXPORT(rKernelExitDeleteThread, 0xE2C40624, 0x1D17DECF);
			FIND_EXPORT(rKernelStartThread, 0xE2C40624, 0x21F5419B);
        } else {
            printf("module data invalid for threadmgr.skprx!\n");
        }
    }
	
	// patch modulemgr
    if (modmgr_idx >= 0) {
        obj = get_obj_for_uid(uids[modmgr_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            if (checkflag(0) == 1) {
				HOOK_EXPORT_CUS(KLoadStartModule, 0xD4A60A52, 0x189BFBBB, KLoadStartModule_Anim);
			} else {
				HOOK_EXPORT_CUS(KLoadStartModule, 0xD4A60A52, 0x189BFBBB, KLoadStartModule_Def);
			}
        } else {
            printf("module data invalid for modulemgr.skprx!\n");
        }
    }
    // patch sdif
    if (sdif_idx >= 0) {
        obj = get_obj_for_uid(uids[sdif_idx]);
        if (obj != NULL) {
            mod = (SceModuleObject *)&obj->data;
            HOOK_EXPORT(sdstor_read_sector_async, 0x96D306FA, 0x6F8D529B);
            HOOK_EXPORT(sdstor_read_sector, 0x96D306FA, 0xB9593652);
            FIND_EXPORT(get_sd_context_part_validate_mmc, 0x96D306FA, 0x6A71987F);
        } else {
            printf("module data invalid for sdif.skprx!\n");
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
        } else {
            printf("module data invalid for authmgr.skprx!\n");
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
                memcpy(mod->segments[0].buf + SYSSTATE_SD0_STRING, sa0_path, sizeof(sa0_path));
                memcpy(mod->segments[0].buf + SYSSTATE_SD0_PSP2CONFIG_STRING, sa0_psp2config_path, sizeof(sa0_psp2config_path));
                // this patch actually corrupts two words of data, but they are only used in debug printing and seem to be fine
                INSTALL_HOOK_THUMB(sysstate_final_hook, mod->segments[0].buf + SYSSTATE_FINAL_CALL);
                sysstate_final = mod->segments[0].buf + SYSSTATE_FINAL;
            );
        } else {
            printf("module data invalid for sysstatemgr.skprx!\n");
        }
    }
    return ret;
}
#undef HOOK_EXPORT
#undef FIND_EXPORT

void go(void) {
    printf("second\n");

    // patch module_load/module_start
    *module_load_func_ptr = module_load_patched;
    printf("module_load_patched: 0x%08X\n", module_load_patched);
}

__attribute__ ((section (".text.start"))) void start(void) {
    go();
}
