/* custom psp2bootconfig reimplementation
 *
 * Copyright (C) 2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdkkern.h>
#include "../patches.h"
#include "../../enso/ex_defs.h"

#define LIST_A_MODULE_COUNT 13
#define LIST_B_MODULE_COUNT 15
#define LIST_C_MODULE_COUNT 15
#define LIST_D_MODULE_COUNT 5

static patch_args_struct patch_args;

static int mlist_uid_a[LIST_A_MODULE_COUNT], mlist_uid_b[LIST_B_MODULE_COUNT], mlist_uid_d[LIST_D_MODULE_COUNT], clist_uid[LIST_C_MODULE_COUNT], use_devmods, use_clist;

static char* clist_str[LIST_C_MODULE_COUNT];
static char e2xlist[16 * 16];

static char *mlist_str_a[] = {
	"sysmem.skprx",
	"excpmgr.skprx",
	"intrmgr.skprx",
	"buserror.skprx",
	"systimer.skprx",
	"acmgr.skprx",
	"threadmgr.skprx",
	"dmacmgr.skprx",
	"smsc_proxy.skprx",
	"authmgr.skprx",
	"processmgr.skprx",
	"iofilemgr.skprx",
	"modulemgr.skprx"
};

static char *mlist_str_b[] = {
	"lowio.skprx",
	"syscon.skprx",
	"oled.skprx",
	NULL,
	"display.skprx",
	"sm_comm.skprx",
	"ss_mgr.skprx",
	"sdif.skprx",
	"msif.skprx",
	"gcauthmgr.skprx",
	"sdstor.skprx",
	"rtc.skprx",
	"exfatfs.skprx",
	"bsod.skprx",
	"sysstatemgr.skprx"
};

static char *mlist_str_d[] = {
	"sdbgsdio.skprx",
	"deci4p_sdfmgr.skprx",
	"deci4p_sttyp.skprx",
	"deci4p_sdbgp.skprx",
	"deci4p_sdrfp.skprx"
};

static void prepare_modlists(void) {
	char dispmodel[16];
	uint32_t cfgstr[16];
	int blist_sz = 0;
	
	// setup arg that will be given to patchers
	cfgstr[0] = 69;
	KblGetHwConfig(&cfgstr); // get some globals from e2x's hwcfg hook
	patch_args.uids_a = mlist_uid_a; // set default list part 1
	patch_args.uids_b = mlist_uid_b; // set default list part 2
	patch_args.uids_d = NULL; // 0 unless devkit (later check)
	patch_args.get_file = (void*)cfgstr[E2X_CHWCFG_GET_FILE]; // e2x's get_file func
	patch_args.nskbl_exports = (void*)cfgstr[E2X_CHWCFG_NSKBL_EXPORTS]; // copy nskbl exports_start addr
	patch_args.kbl_param = (void*)cfgstr[E2X_CHWCFG_KBLPARAM];
	patch_args.load_exe = (void*)cfgstr[E2X_CHWCFG_LOAD_EXE]; // e2x's load_exe func
	patch_args.ctrldata = cfgstr[E2X_CHWCFG_CTRL]; // current ctrl status
	
	// custom
	int (*get_file)(char* file_path, void* buf, uint32_t read_size, uint32_t offset) = patch_args.get_file;
	if (CTRL_BUTTON_HELD(patch_args.ctrldata, E2X_EPATCHES_SKIP)) {
		clist_str[0] = "e2xrecovr.skprx"; // recovery script
		clist_str[1] = "e2xhfwmod.skprx"; // always run the hfw-compat script
		use_clist = 2;
	} else { // TODO: REWORK
		blist_sz = get_file("os0:ex/boot_list.txt", NULL, 0, 0); // bootlist size
		if (blist_sz > 0 && get_file("os0:ex/boot_list.txt", e2xlist, (blist_sz < 0x100) ? blist_sz : 0x100, 0) == 0 && (e2xlist[2] == 'X') && (e2xlist[12] == '=')) { // "E2XMODCOUNT:=XX\0"
			for (int i = 1; i < (((e2xlist[13] - 0x30) * 10) + (e2xlist[14] - 0x30) + 1); i-=-1) {
				if (i > 14)
					break;
				clist_str[i - 1] = &e2xlist[i * 16];
				e2xlist[(i * 16) + 15] = 0x00;
				use_clist = i;
			}
		} else {
			clist_str[0] = "e2xhencfg.skprx";
			clist_str[1] = "e2xculogo.skprx";
			clist_str[2] = "e2xhfwmod.skprx";
			use_clist = 3;
		}
	}
	
	// devkit
	int iVar1 = KblCheckDipsw(0xc1);
	if ((((iVar1 != 0) && (iVar1 = KblIsCex(), iVar1 == 0)) && (iVar1 = KblIsModelx102(), iVar1 == 0)) && (iVar1 = KblIsDoubleModel(), iVar1 == 0)) {
		use_devmods = 5;
		iVar1 = KblParamx2dAnd1();
		if (iVar1 == 0) {
			use_devmods = 3;
			mlist_str_d[3] = NULL;
			mlist_str_d[4] = NULL;
		}
		patch_args.uids_d = mlist_uid_d;
	} else
		use_devmods = 0;
	
	// can bsod
	if (KblIsCex())
		mlist_str_b[13] = NULL;
	
	// display type
	iVar1 = KblIsGenuineDolce();
	if (iVar1 == 0) {
		KblGetHwConfig(&dispmodel);
		if ((dispmodel[0] & 9) != 0)
			mlist_str_b[2] = "lcd.skprx";
	} else
		mlist_str_b[2] = NULL;
	
	// has hdmi
	iVar1 = KblIsNotDolce();
	if ((iVar1 == 0) || ((iVar1 = KblIsCex(), iVar1 == 0 && (iVar1 = KblIsModelx102(), iVar1 == 0))))
		mlist_str_b[3] = "hdmi.skprx";
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, void *args) {
	
	// prepare all the module lists
	prepare_modlists();
	
	// load default modules
	KblLoadModulesFromList(mlist_str_a, mlist_uid_a, LIST_A_MODULE_COUNT, 0);
	if (use_devmods > 0) // devkit modules
		KblLoadModulesFromList(mlist_str_d, mlist_uid_d, use_devmods, KblCheckDipsw(0xd2) ? 1 : 0);
	KblLoadModulesFromList(mlist_str_b, mlist_uid_b, LIST_B_MODULE_COUNT, 0);
	
	// change moddir to os0:ex/ and load custom modules from there
	if (use_clist > 0) {
		*(uint8_t *)0x51023de7 = 'e';
		*(uint8_t *)0x51023de8 = 'x';
		KblLoadModulesFromList(clist_str, clist_uid, use_clist, 0);
	}
	
	// cleanup
	KblDeinitStorageHandlers();
	KblStopKASM();
	
	return 0;
}

void _stop() __attribute__ ((weak, alias ("module_bootstart")));
int module_bootstart(SceSize argc, void *args) {
	// set some sysroot stuff
	uint32_t tmpr = 0;
	KblDisableIrq();
	*(uint32_t *)(args + 0x2fc) = 0;
	*(uint32_t *)(args + 0x300) = 0;
	tmpr = KblGetDevRegs();
	*(uint32_t *)(args + 0x304) = tmpr;
	tmpr = KblGetDipsw204And205Stat();
	*(uint32_t *)(args + 0x308) = tmpr;
	
	// start the custom modules
	patch_args.defarg = args;
	if (use_clist > 0)
		KblStartModulesFromList(clist_uid, use_clist, 4, &patch_args);
	
	// start default modules part 1
	KblStartModulesFromList(mlist_uid_a, LIST_A_MODULE_COUNT, 4, args);
	if (use_devmods > 0)
		KblStartModulesFromList(mlist_uid_d, (use_devmods == 3) ? 3 : 4, 0, 0);
	
	// cleanup, prepare for sysstatemgr boot handoff
	KblExecSysrootx2d4();
	KblWriteModStartRegs();
	KblExecSysrootx360x20();
	KblExecSysrootx2d8();
	
	// start default modules part 2
	if (use_devmods == 5)
		KblStartModulesFromList(&mlist_uid_d[4], 1, 0, 0);
	KblStartModulesFromList(mlist_uid_b, LIST_B_MODULE_COUNT, 4, args);
	
	return 0;
}