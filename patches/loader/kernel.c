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

static patch_args_struct patch_args;

static int mlist_uid_a[13], mlist_uid_b[15], mlist_uid_d[5], clist_uid[15], use_devmods, use_clist;

static char *clist_str[15];
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
	
	// setup arg that will be given to patchers
	cfgstr[0] = 69;
	KblGetHwConfig(&cfgstr); // get some globals from e2x's hwcfg hook
	patch_args.uids_a = mlist_uid_a; // set default list part 1
	patch_args.uids_b = mlist_uid_b; // set default list part 2
	patch_args.uids_d = NULL; // 0 unless devkit (later check)
	patch_args.nskbl_exports = (void *)cfgstr[3]; // copy nskbl exports_start addr
	patch_args.kbl_param = (void *)cfgstr[2];
	patch_args.load_file = (void *)cfgstr[1]; // e2x's load_file func
	patch_args.ctrldata = cfgstr[0]; // current ctrl status
	
	// custom
	int (*lf)(char *fpath, void *dst, unsigned int sz, int mode) = patch_args.load_file;
	if (CTRL_BUTTON_HELD(patch_args.ctrldata, E2X_EPATCHES_SKIP)) {
		clist_str[0] = "e2xrecovr.skprx"; // recovery script
		clist_str[1] = "e2xhfwmod.skprx"; // always run the hfw-compat script
		use_clist = 2;
	} else {
		if ((lf("os0:ex/boot_list.txt", e2xlist, (16 * 16), 2) == 0) && ((e2xlist[0] == 'E') && (e2xlist[2] == 'X') && (e2xlist[12] == '='))) { // "E2XMODCOUNT:=XX\0"
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
	KblLoadModulesFromList(mlist_str_a, mlist_uid_a, 13, 0);
	if (use_devmods > 0) // devkit modules
		KblLoadModulesFromList(mlist_str_d, mlist_uid_d, use_devmods, KblCheckDipsw(0xd2) ? 1 : 0);
	KblLoadModulesFromList(mlist_str_b, mlist_uid_b, 15, 0);
	
	// change moddir to os0:ex/ and load custom modules from there
	if (use_clist > 0) {
		if (*(uint32_t *)(patch_args.kbl_param + 0x4) == 0x03650000) {
			*(uint8_t *)0x51023de7 = 'e';
			*(uint8_t *)0x51023de8 = 'x';
		} else {
			*(uint8_t *)0x51023bdf = 'e';
			*(uint8_t *)0x51023be0 = 'x';
		}
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
	KblStartModulesFromList(mlist_uid_a, 13, 4, args);
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
	KblStartModulesFromList(mlist_uid_b, 15, 4, args);
	
	return 0;
}