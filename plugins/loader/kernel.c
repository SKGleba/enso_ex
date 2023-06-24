/* custom psp2bootconfig reimplementation
 *
 * Copyright (C) 2020-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdkkern.h>
#include "../plugins.h"
#include "../../core/ex_defs.h"

#define LIST_A_MODULE_COUNT 13
#define LIST_B_MODULE_COUNT 15
#define LIST_C_MODULE_COUNT E2X_MAX_EPATCHES_N
#define LIST_D_MODULE_COUNT 5

#define CUSTOM_BOOT_LIST "os0:ex/boot_list.txt"
#define CUSTOM_BOOT_LIST_MAGIC 'LXE#' // #EXL
static int bootlist_mb_id = -1;

static patch_args_struct patch_args;

static int mlist_uid_a[LIST_A_MODULE_COUNT], mlist_uid_b[LIST_B_MODULE_COUNT], mlist_uid_d[LIST_D_MODULE_COUNT], clist_uid[LIST_C_MODULE_COUNT];
static int use_devmods = 0, use_clist = 0;

static char* clist_str[LIST_C_MODULE_COUNT];

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

char* find_endline(char* start, char* end) {
	for (char* ret = start; ret < end; ret++) {
		if (*(uint16_t*)ret == 0x0A0D || *(uint8_t*)ret == 0x0A)
			return ret;
	}
	return end;
}

char* find_nextline(char* current_line_end, char* end) {
	for (char* next_line = current_line_end; next_line < end; next_line++) {
		if (*(uint8_t*)next_line != 0x0D && *(uint8_t*)next_line != 0x0A && *(uint8_t*)next_line != 0x00)
			return next_line;
	}
	return NULL;
}

void parse_bootlist(void* data_start, int data_size) {
	char* startlist = data_start;
	char* endlist = startlist + data_size;

	char* current_line = startlist;
	char* end_line = startlist;
	while (current_line < endlist) {
		end_line = find_endline(current_line, endlist);
		if (current_line[0] != '#' && current_line != end_line) {
			for (int i = 0; i < (end_line - current_line); i++) {
				if (current_line[i] == ' ' || current_line[i] == '#') {
					*(uint8_t*)(current_line + i) = 0;
					break;
				}
			}
			end_line[0] = 0;
			clist_str[use_clist++] = current_line;
		}
		current_line = find_nextline(end_line, endlist);
		if (!current_line || use_clist >= LIST_C_MODULE_COUNT)
			break;
	}
	return;
}

static void prepare_modlists(char** module_dir_s) {
	char dispmodel[16];

	// get enso_ex funcs & useful data
	patchedHwcfgStruct *cfg = &patch_args;
	cfg->get_ex_ports = E2X_MAGIC;
	if (KblGetHwConfig(cfg) == E2X_MAGIC) { // core version check
		// module directory string paddr
		*module_dir_s = cfg->ex_ports.module_dir;
		
		// setup arg that will be given to patchers
		patch_args.uids_a = mlist_uid_a; // set default list part 1
		patch_args.uids_b = mlist_uid_b; // set default list part 2
		patch_args.uids_d = NULL; // 0 unless devkit (later check)

		// custom
		if (CTRL_BUTTON_HELD(patch_args.ex_ctrl, E2X_EPATCHES_SKIP)) {
			clist_str[0] = "e2xrecovr.skprx"; // recovery script
			clist_str[1] = "e2xhfwmod.skprx"; // always run the hfw-compat script
			use_clist = 2;
		} else {
			int bootlist_size = (int)patch_args.ex_get_file(CUSTOM_BOOT_LIST, NULL, 0, 0);
			void *bootlist = (bootlist_size > 0) ? patch_args.ex_load_exe(CUSTOM_BOOT_LIST, "boot_list", 0, (uint32_t)bootlist_size, E2X_LX_NO_XREMAP | E2X_LX_NO_CCACHE, &bootlist_mb_id) : NULL;
			if (bootlist && *(uint32_t*)bootlist == CUSTOM_BOOT_LIST_MAGIC) { // ensure we dont use the old list
				if (bootlist_size & 0xFFF)
					*(uint8_t*)(bootlist + bootlist_size) = 0;
				else
					*(uint8_t*)(bootlist + bootlist_size - 1) = 0; // possibly cut last entry
				parse_bootlist(bootlist, bootlist_size);
			} else {
				clist_str[0] = "e2xhencfg.skprx";
				clist_str[1] = "e2xculogo.skprx";
				clist_str[2] = "e2xhfwmod.skprx";
				use_clist = 3;
			}
		}
	} else
		use_clist = 0;

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
int module_start(SceSize argc, void* args) {

	char* module_dir = NULL;

	// prepare all the module lists
	prepare_modlists(&module_dir);
	
	// load default modules
	KblLoadModulesFromList(mlist_str_a, mlist_uid_a, LIST_A_MODULE_COUNT, 0);
	if (use_devmods > 0) // devkit modules
		KblLoadModulesFromList(mlist_str_d, mlist_uid_d, use_devmods, KblCheckDipsw(0xd2) ? 1 : 0);
	KblLoadModulesFromList(mlist_str_b, mlist_uid_b, LIST_B_MODULE_COUNT, 0);
	
	// change moddir to os0:ex/ and load custom modules from there
	if (use_clist > 0 && module_dir) {
		module_dir[0] = 'e';
		module_dir[1] = 'x';
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
	patch_args.this_version = PATCH_ARGS_VERSION; // loader's this struct version
	patch_args.defarg = args;
	if (use_clist > 0) {
		KblStartModulesFromList(clist_uid, use_clist, 4, &patch_args);
		if (bootlist_mb_id != -1)
			patch_args.kbl_free_memblock(bootlist_mb_id);
	}

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