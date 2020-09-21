#include <inttypes.h>
#include <stdio.h>

#ifdef FW_365
	#include "../../../enso/365/nsbl.h"
#else
	#include "../../../enso/360/nsbl.h"
#endif

#include "../../../enso/ex_defs.h"

// for all I/O funcs: FUNC(dev_off_blk, mem_buf, blk_count)
#define MMCWRITE(...) write_sector_mmc((int *)*(uint32_t *)0x51028014, ##__VA_ARGS__)
#define MMCREAD(...) read_sector_mmc_direct((int *)*(uint32_t *)0x51028014, ##__VA_ARGS__)
#define SDREAD(...) read_sector_sd((int *)*(uint32_t *)0x5102801c, ##__VA_ARGS__)
#define SDWRITE(...) \
do { \
	set_sd_write_mode(1); \
	SDREAD(__VA_ARGS__); \
	set_sd_write_mode(0); \
} while (0)
	
#define CONCAT44(high, low) ((((uint64_t) high) << 32) | ((uint64_t) low))
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef struct {
	uint32_t off;
	uint32_t sz;
	uint8_t code;
	uint8_t type;
	uint8_t active;
	uint32_t flags;
	uint16_t unk;
} __attribute__((packed)) partition_t;

typedef struct {
	char magic[0x20];
	uint32_t version;
	uint32_t device_size;
	char unk1[0x28];
	partition_t partitions[0x10];
	char unk2[0x5e];
	char unk3[0x10 * 4];
	uint16_t sig;
} __attribute__((packed)) master_block_t;

// nskbl funcs
#ifdef FW_365
static int (*read_sector_mmc_direct)(int *ctx, unsigned int block_offset, uint32_t target_buf, int block_count) = (void*)0x5101c515;
static int (*lsdif_mmc_verify_args)(int *ctx, unsigned int block_offset, int block_count) = (void*)0x5101c23d;
static int (*lsdif_mmc_prep_ctx)() = (void*)0x5101c091;
static int (*lsysclib_concat_unk)() = (void*)0x510221fc;
static int (*lsdif_mmc_prepare_args)() = (void*)0x5101bbf9;
static int (*lsdif_mmc_write_args)() = (void*)0x5101c0a5;
static int (*lsdif_ctrl_apply_cmd)() = (void*)0x5101bf65;
#else
static int (*read_sector_mmc_direct)(int *ctx, unsigned int block_offset, uint32_t target_buf, int block_count) = (void*)0x5101c30d;
static int (*lsdif_mmc_verify_args)(int *ctx, unsigned int block_offset, int block_count) = (void*)0x5101c035;
static int (*lsdif_mmc_prep_ctx)() = (void*)0x5101be89;
static int (*lsysclib_concat_unk)() = (void*)0x51021ff4;
static int (*lsdif_mmc_prepare_args)() = (void*)0x5101b9f5;
static int (*lsdif_mmc_write_args)() = (void*)0x5101be9d;
static int (*lsdif_ctrl_apply_cmd)() = (void*)0x5101bd5d;
#endif

// patch read_sector_sd to send a write command instead
#ifdef FW_365
static void set_sd_write_mode(int mode) {
	if (mode) {
		*(uint8_t *)0x5101e8b7 = 0x62;
		*(uint8_t *)0x5101e8bb = 0x60;
		*(uint8_t *)0x5101e8c2 = 0x18;
		*(uint8_t *)0x5101e8c4 = 0x19;
		clean_dcache((void *)0x5101e8b0, 0x20);
		flush_icache();
	} else { // undo patches
		*(uint8_t *)0x5101e8b7 = 0x52;
		*(uint8_t *)0x5101e8bb = 0x50;
		*(uint8_t *)0x5101e8c2 = 0x11;
		*(uint8_t *)0x5101e8c4 = 0x12;
		clean_dcache((void *)0x5101e8b0, 0x20);
		flush_icache();
	}
}
#else
static void set_sd_write_mode(int mode) {
	if (mode) {
		*(uint8_t *)0x5101e6af = 0x62;
		*(uint8_t *)0x5101e6b3 = 0x60;
		*(uint8_t *)0x5101e6ba = 0x18;
		*(uint8_t *)0x5101e6bc = 0x19;
		clean_dcache((void *)0x5101e6a0, 0x20);
		flush_icache();
	} else { // undo patches
		*(uint8_t *)0x5101e6af = 0x52;
		*(uint8_t *)0x5101e6b3 = 0x50;
		*(uint8_t *)0x5101e6ba = 0x11;
		*(uint8_t *)0x5101e6bc = 0x12;
		clean_dcache((void *)0x5101e6a0, 0x20);
		flush_icache();
	}
}
#endif

// write [block_count] from [target_buf] to MMC @ [block_offset] for ctx [ctx]
static int write_sector_mmc(int *ctx, unsigned int block_offset, int target_buf, int block_count) {
	int ret, unkcmd1, ctxs2, unkcmd3;;
	uint32_t *ctxspav, *ctxspav_2;
	unsigned int *ctxdd, unkcmd0, unkcmd2, ctxdd_a, ctxdd_b, rand[4];
	long long int daddr;
	unsigned short ctx2s;
	
	if (((target_buf == 0 || ctx == NULL) || (block_count == 0)) || (ctxs2 = *ctx, ctxs2 == 0)) {
		ret = -1;
		goto EXIT;
	}
	
	ret = lsdif_mmc_verify_args(ctx, block_offset, block_count);
	
	if (ret < 0)
		goto EXIT;
	
	ctxspav = (uint32_t *)lsdif_mmc_prep_ctx(ctxs2);
	
	if (ctxspav == NULL) {
RBAD_ARGS:
		ret = -2;
		goto EXIT;
	}
	
	if (block_count == 1)
		ret = 0x18;
	else
		ret = 0x19;
	
	unkcmd0 = ctx[1];
	*ctxspav = 0x240;
	ctxspav[1] = 0x614;
	ctxspav[2] = ret;
	ctx2s = *(unsigned short *)(ctx + 2);
	unkcmd2 = (uint32_t)ctx2s;
	ctxdd = (unsigned int *)ctx[0xe4];
	ctxdd_a = ctxdd[4];
	ctxdd_b = *ctxdd;
	daddr = (unsigned long long int)unkcmd2 * (unsigned long long int)block_offset + CONCAT44(ctxdd[5] - (ctxdd[1] + (uint32_t)(ctxdd_a < ctxdd_b)), ctxdd_a - ctxdd_b);
	unkcmd1 = lsysclib_concat_unk((int)daddr, (int)((unsigned long long int)daddr >> 0x20), unkcmd2, 0);
	
	if ((unkcmd0 & 1) == 0)
		unkcmd1 = unkcmd1 << 9;
	
	ctxspav[3] = unkcmd1;
	ctxspav[8] = target_buf;
	*(unsigned short *)(ctxspav + 9) = ctx2s;
	*(uint16_t *)((int)ctxspav + 0x26) = (short)block_count;
	
	if (ret == 0x19) {
		ctxspav_2 = (uint32_t *)lsdif_mmc_prep_ctx(ctxs2);
		
		if (ctxspav_2 == NULL) {
			lsdif_mmc_write_args(ctxs2, ctxspav);
			goto RBAD_ARGS;
		}
		
		ctxspav_2[3] = block_count;
		*ctxspav_2 = 0x240;
		ctxspav_2[1] = 0x13;
		ctxspav_2[2] = 0x17;
		ctxspav_2[8] = 0;
		ret = lsdif_mmc_prepare_args(ctxs2, ctxspav_2, ctxspav,3);
		
		if (ret < 0) {
			lsdif_mmc_write_args(ctxs2, ctxspav_2);
			goto DRDY_ERR;
		}
		
		unkcmd0 = ctxspav[0x6e];
		unkcmd1 = ctxspav[0x6f];
		unkcmd2 = ctxspav[0x6c];
		unkcmd3 = ctxspav[0x6d];
		ret = *ctx;
		*(int *)(ctxs2 + 0x2498) = unkcmd0 - unkcmd2;
		*(int *)(ctxs2 + 0x249c) = unkcmd1 - (unkcmd3 + (unsigned int)(unkcmd0 < unkcmd2));
		ret = lsdif_ctrl_apply_cmd(ret, rand);
		
		if (-1 < ret) {
			lsdif_mmc_write_args(ctxs2, ctxspav_2);
			goto CAOP_END;
		}
		
		lsdif_mmc_write_args(ctxs2, ctxspav_2);
		lsdif_mmc_write_args(ctxs2, ctxspav);
	} else {
		ret = lsdif_mmc_prepare_args(ctxs2, ctxspav, 0, 3);
		
		if (-1 < ret) {
			unkcmd0 = ctxspav[0x6e];
			unkcmd1 = ctxspav[0x6f];
			unkcmd2 = ctxspav[0x6c];
			unkcmd3 = ctxspav[0x6d];
			ret = *ctx;
			*(int *)(ctxs2 + 0x2498) = unkcmd0 - unkcmd2;
			*(int *)(ctxs2 + 0x249c) = unkcmd1 - (unkcmd3 + (uint32_t)(unkcmd0 < unkcmd2));
			ret = lsdif_ctrl_apply_cmd(ret, rand);
			
			if (-1 < ret) {
CAOP_END:
				lsdif_mmc_write_args(ctxs2, ctxspav);
				
				if ((*(char *)(ctxs2 + 0x2439) == 1) && (*(int *)(ctxs2 + 0x2420) == 1)) {
					*(char *)(ctxs2 + 0x243b) = *(char *)(ctxs2 + 0x243b) + 1;
				}
				
				ret = 0;
				goto EXIT;
			}
		}
DRDY_ERR:
		lsdif_mmc_write_args(ctxs2, ctxspav);
	}
EXIT:
	return ret;
}

static int get_act_os_off(master_block_t *master) {
	int off = 0;
	for (size_t i = 0; i < ARRAYSIZE(master->partitions); ++i) {
		partition_t *p = &master->partitions[i];
		if (p->active == 1 && p->code == 3)
			off = p->off;
	}
	return off;
}

int __attribute__((optimize("O0"))) start(void *kbl_param, unsigned int ctrldata) {
	// prepare buf for comm
	uint32_t buf[0x80];
	memset(&buf, 0, 0x200);
	SDWRITE(0, (int)&buf, 1);
	
	// get active os0 off (from custom MBR)
	MMCREAD(1, (int)&buf, 1);
	uint32_t act_os_off = get_act_os_off((master_block_t *)&buf);
	if ((act_os_off < 0x8000) || (act_os_off > 0x10000))
		act_os_off = 0x8000;
	
	// ina os0 off
	uint32_t ina_os_off = 0x10000;
	if (ina_os_off == act_os_off)
		ina_os_off = 0x8000;
	
	// dump mbr to SD
	buf[0] = (uint32_t)0xdeadbeef;
	SDWRITE(0, (int)&buf, 1);
	
	// 16 MiB memcock for os0
	int buf_block = sceKernelAllocMemBlock("cpybuf", 0x10208006, 0x8000 * 0x200, 0);
	if (buf_block < 0) // make sure that its alloced
		return 0;
	void *buf_block_addr = NULL;
	sceKernelGetMemBlockBase(buf_block, (void**)&buf_block_addr);
	
	// flash os0
	int rret = SDREAD(act_os_off, (int)buf_block_addr, 0x8000); // SD os0->act os0
	//int rret = MMCREAD(ina_os_off, (int)buf_block_addr, 0x8000); // ina os0->act os0
	int wret = MMCWRITE(act_os_off, (int)buf_block_addr, 0x8000);
	
	// if SDIO controller dies at port reset do this: flash first 16 MiB of vs0 to prevent bootloop
	// int vret = MMCWRITE(0x58000, (int)buf_block_addr, 0x8000);
	
	sceKernelFreeMemBlock(buf_block);
	
	// write back results
	buf[0] = (uint32_t)0xcafebeef;
	buf[1] = act_os_off;
	buf[2] = ina_os_off;
	buf[3] = (uint32_t)rret;
	buf[4] = (uint32_t)wret;
	//buf[5] = (uint32_t)vret;
	SDWRITE(0, (int)&buf, 1);
	return 0;
}