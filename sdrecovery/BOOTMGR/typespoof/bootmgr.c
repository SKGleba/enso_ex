// typespoof_e2xs

#include <inttypes.h>
#include <stdio.h>

#ifdef FW_365
	#include "../../../enso/365/nsbl.h"
#else
	#include "../../../enso/360/nsbl.h"
#endif

#include "../../../enso/ex_defs.h"

// per-fw offsets
#ifdef FW_365
	#define LOADSM_KA_NOP_1 0x51016cf2
	#define LOADSM_KA_NOP_2 0x51016cf4
	#define LOADSM_KA_NOP_R 0x51016cf0
	#define CALL_KA_NOP_A 0x51016bec
	#define CALL_KA_NOP_R 0x51016be0
#else
	#define LOADSM_KA_NOP_1 0x51016bb6
	#define LOADSM_KA_NOP_2 0x51016bb8
	#define LOADSM_KA_NOP_R 0x51016bb0
	#define CALL_KA_NOP_A 0x51016ab0
	#define CALL_KA_NOP_R 0x51016ab0
#endif

#define CEX 0x0301 // retail
#define DEX 0x0201 // testkit
#define DEVTOOL 0x0101 // devkit
#define TEST 0x0001 // system debugger (internal)

#define SPOOFED_TYPE DEX

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

// patch re-write keyslot 0x509 patching device type
static const unsigned char dex_patch_nmp[] = {
  0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3, 0x24, 0x21, 0xe0, 0x6f, 0x3e, 0x03,
  0x06, 0x43, 0x21, 0xc3, 0x85, 0x1f, 0x34, 0xc3, 0x00, 0x03, 0x3f, 0x03,
  0xf9, 0xc3, 0x04, 0x00, 0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3, 0x20, 0x21,
  0x3e, 0x02, 0x21, 0xc3, 0x03, 0xe0, 0x3a, 0x02, 0x07, 0x42, 0x34, 0xc3,
  0x04, 0x00, 0x20, 0x6f, 0x3a, 0x02, 0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3,
  0x28, 0x21, 0x3e, 0x02, 0x21, 0xc3, 0x03, 0xe0, 0x34, 0xc3, 0x08, 0x00,
  0x3a, 0x02, 0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3, 0x2c, 0x21, 0x3e, 0x02,
  0x21, 0xc3, 0x03, 0xe0, 0x34, 0xc3, 0x0c, 0x00, 0x3a, 0x02, 0x21, 0xc3,
  0x06, 0xe0, 0x34, 0xc3, 0x30, 0x21, 0x3e, 0x02, 0x21, 0xc3, 0x03, 0xe0,
  0x34, 0xc3, 0x10, 0x00, 0x3a, 0x02, 0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3,
  0x34, 0x21, 0x3e, 0x02, 0x21, 0xc3, 0x03, 0xe0, 0x34, 0xc3, 0x14, 0x00,
  0x3a, 0x02, 0x21, 0xc3, 0x06, 0xe0, 0x34, 0xc3, 0x38, 0x21, 0x3e, 0x02,
  0x21, 0xc3, 0x03, 0xe0, 0x34, 0xc3, 0x18, 0x00, 0x3a, 0x02, 0x21, 0xc3,
  0x06, 0xe0, 0x34, 0xc3, 0x3c, 0x21, 0x3e, 0x02, 0x21, 0xc3, 0x03, 0xe0,
  0x34, 0xc3, 0x1c, 0x00, 0x3a, 0x02, 0x21, 0xc3, 0x03, 0xe0, 0x34, 0xc3,
  0x20, 0x00, 0x01, 0xc2, 0x09, 0x05, 0x3a, 0x02, 0x02, 0x70, 0x00, 0x00,
  0xf0, 0xff, 0x1f, 0x00
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
#else
static int (*smtool_invoke)(uint32_t param_1, uint32_t self_paddr_list_paddr, uint32_t self_paddr_list_count, uint32_t *param_4, SceSblSmCommContext130 *param_5, int *id) = (void*)0x51015de5;
static int (*smtool_call_big_ka)(void *argv, uint32_t args, uint32_t cmdid) = (void*)0x51016a8d;
static int (*smtool_stop)(void) = (void*)0x51016f29;
static int (*smtool_load_ka)(void) = (void*)0x51016b1d;
static int (*get_paddr_single)(void *va, void *pa) = (void*)0x5100788d;
static int (*get_paddr_list)(void *vrange, void *palist) = (void*)0x510078a5;
#endif

// read file to buffer
static int read_f2b(unsigned int size, void *buf, char *file) {
	long long int Var11 = iof_open(file, 1, 0);
	if (Var11 < 0)
		return 1;
	uint32_t uVar4 = (uint32_t)((unsigned long long int)Var11 >> 0x20);
	Var11 = iof_get_sz(uVar4, (int)Var11, 0, 0, 2);
	if (Var11 < 0)
		return 1;
	//unsigned int fsz = (unsigned int)((unsigned long long int)Var11 >> 0x20); -- bugged for bigger files for some reason
	unsigned int fsz = size;
	if ((int)iof_get_sz(uVar4, (int)Var11, 0, 0, 0) >= 0 && iof_read(uVar4, buf, &fsz) >= 0) {
		if (*(uint32_t *)buf == 0) {
			iof_close(uVar4);
			return 2;
		}
		iof_close(uVar4);
		return 0;
	}
	iof_close(uVar4);
	return 3;
}

static int load_ussm(int *ctx) {
	unsigned int smsz = 42456;
	int sm_block = sceKernelAllocMemBlock("sm_cached", 0x10208006, (smsz + 0xfff) & 0xfffff000, 0), cmd_block = sceKernelAllocMemBlock("load_sm_cmd", 0x10208006, 0x1000, 0);
	void *sm_block_addr = NULL, *cmd_block_addr = NULL;
	sceKernelGetMemBlockBase(sm_block, (void**)&sm_block_addr); // block for SM
	sceKernelGetMemBlockBase(cmd_block, (void**)&cmd_block_addr); // block for SM args
	if (read_f2b(smsz, sm_block_addr, "os0:sm/update_service_sm.self") > 0)
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
	void *args_block_addr = NULL, *payload_block_addr = NULL;
	sceKernelGetMemBlockBase(args_block, (void**)&args_block_addr); // for SM args
	sceKernelGetMemBlockBase(payload_block, (void**)&payload_block_addr); // for payload(s)
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
	memcpy((payload_block_addr + 0x100), &dex_patch_nmp, sizeof(dex_patch_nmp)); // type patch installer
	*(uint16_t *)(payload_block_addr + 0x300) = (uint16_t)SPOOFED_TYPE;
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
	sceKernelFreeMemBlock(payload_block);
	return;
}

void gp_main(void) {
	if (load_ussm((int *)0x5113f0e8) == 0) { // load update_sm
		cmep_run(); // add type patches
		smtool_stop(); // stop the sm to allow kprxauth load
		*(uint16_t *)(boot_args + 0xA2) = (uint16_t)SPOOFED_TYPE; // update enso's KBL_PARAM.DTYPE for patches compat
		*(uint16_t *)((*(uint32_t *)(*(uint32_t *)(0x51138a3c) + 0x6c)) + 0xA2) = (uint16_t)SPOOFED_TYPE; // update sysrootNP2->KBL_PARAM.DTYPE to bypass mismatch
	}
	return;
}

__attribute__ ((section (".text.start"))) void start(void) {
	gp_main();
	return;
}