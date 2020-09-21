/* 
	TypeSpoof by SKGleba
	All Rights Reserved
*/

#include <stdio.h>
#include <string.h>
#include <taihen.h>
#include <psp2kern/kernel/modulemgr.h>
#include <vitasdkkern.h>
#include "../../../../../psp2renga/Include/nmprunner.h"
#include "../payload/dex_patch.h"

#define CEX 0x0301 // retail
#define DEX 0x0201 // testkit
#define DEVTOOL 0x0101 // devkit
#define TEST 0x0001 // system debugger (internal)

#define SPOOFED_TYPE DEX

// flag if patched load_sm should patch type ASAP
static int should_patch = 0;

// A mod of NMPrun_default
static int patch_type(uint32_t fw) {
	int ret = 0;
	NMPctx = -1;
	ret = NMPexploit_init(fw);
	if (ret != 0)
		return ret;
	ret = NMPconfigure_stage2(fw);
	if (ret != 0)
		return (0x60 + ret);
	ret = NMPreserve_commem(0, 1);
	if (ret != 0)
		return (0x10 + ret);
	char bkp[0x80];
	memcpy(&bkp, NMPcorridor, 0x80); // save first 0x80 bytes of SPRAM
	memset(NMPcorridor, 0, 0x300);
	ret = NMPcopy(&NMPstage2_payload, 0, sizeof(NMPstage2_payload), 0);
	if (ret != 0)
		return (0x20 + ret);
	ret = NMPcopy(&dex_patch_nmp, 0x100, dex_patch_nmp_len, 0);
	if (ret != 0)
		return (0x30 + ret);
	*(uint16_t *)(NMPcorridor + 0x300) = (uint16_t)SPOOFED_TYPE;
	ret = NMPf00d_jump((uint32_t)NMPcorridor_paddr, fw);
	if (ret != 0)
		return (0x40 + ret);
	memcpy(NMPcorridor, &bkp, 0x80); // restore first 0x80 bytes of SPRAM (WHY? something dies)
	ret = NMPfree_commem(0);
	if (ret != 0)
		return (0x50 + ret);
	ksceSblSmCommStopSm(NMPctx, &NMPstop_res);
	return 0;
}

// If the flag is set - run the type_patch payload
static tai_hook_ref_t load_sm_data_ref;
static int load_sm_data_hook(int a0, int a1, int a2, int a3, int a4, int a5) {
	if (should_patch == 1) {
		should_patch = 0;
		patch_type(0x03650000); // lv0 offsets are the same for 3.60-3.65
	}
	return TAI_CONTINUE(int, load_sm_data_ref, a0, a1, a2, a3, a4, a5);
}

// 0x10000 is the first sysevent that is passed, make load_sm hook patch type ASAP
static int ts_sysevent_handler(int resume, int eventid, void *args, void *opt) {
	if ((resume) && (eventid == 0x10000))
		should_patch = 1;
	return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
	// Enable origin fw update_sm caching
	NMPcache_ussm("os0:sm/update_service_sm.self", 1);
	
	// Use venezia SPRAM
	NMPcorridor_paddr = 0x1f850000;
	NMPcorridor_size = 0x10000;
	
	ksceKernelRegisterSysEventHandler("ts_sysevent", ts_sysevent_handler, NULL);
	
	taiHookFunctionExportForKernel(0x10005, &load_sm_data_ref, "SceSblSsSmComm", 0xCD3C89B6, 0x039C73B1, load_sm_data_hook);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}
