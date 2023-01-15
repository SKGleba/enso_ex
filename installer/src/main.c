/* main.c -- enso installer (user side)
 *
 * Copyright (C) 2017 molecule, 2018-2020 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <taihen.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "enso.h"
#include "version.h"

#include "graphics.h"

#include "../../enso/ex_defs.h"

#define printf psvDebugScreenPrintf
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

int vshPowerRequestColdReset(void);
int curfw = 69;

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	PROGRESS_BAR_WIDTH = SCREEN_WIDTH,
	PROGRESS_BAR_HEIGHT = 10,
	LINE_SIZE = SCREEN_WIDTH,
};

static unsigned buttons[] = {
	SCE_CTRL_SELECT,
	SCE_CTRL_START,
	SCE_CTRL_UP,
	SCE_CTRL_RIGHT,
	SCE_CTRL_DOWN,
	SCE_CTRL_LEFT,
	SCE_CTRL_LTRIGGER,
	SCE_CTRL_RTRIGGER,
	SCE_CTRL_TRIANGLE,
	SCE_CTRL_CIRCLE,
	SCE_CTRL_CROSS,
	SCE_CTRL_SQUARE,
};

int fap(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");
	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);
	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);
	FILE *pFile = fopen(to, "ab");
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
	fclose(fp);
	fclose(pFile);
	return 1;
}

int fcp(const char *from, const char *to) {
	SceUID fd = sceIoOpen(to, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 6);
	sceIoClose(fd);
	int ret = fap(from, to);
	return ret;
}

int ex(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int getsz(char *fname)
{
	SceUID fd = sceIoOpen(fname, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoClose(fd);
	return fileSize;
}

void copyDir(char *src, char *dst) {
	SceUID dfd = sceIoDopen(src);
	if (dfd >= 0) {
		int res = 0;
		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));
			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				psvDebugScreenSetFgColor(COLOR_PURPLE);
				printf("  -  %s... ", dir.d_name);
				char *src_path = malloc(strlen(src) + strlen(dir.d_name) + 2);
				snprintf(src_path, 255, "%s%s", src, dir.d_name);
				char *dst_path = malloc(strlen(dst) + strlen(dir.d_name) + 2);
				snprintf(dst_path, 255, "%s%s", dst, dir.d_name);
				fcp(src_path, dst_path);
				psvDebugScreenSetFgColor(COLOR_GREEN);
				printf("ok!\n");
				psvDebugScreenSetFgColor(COLOR_WHITE);
			}
		} while (res > 0);
		sceIoDclose(dfd);
	}
	return;
}

uint32_t get_key(void) {
	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned new = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (size_t i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (new & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

void press_exit(void) {
	printf("Press any key to exit this application.\n");
	get_key();
	sceKernelExitProcess(0);
}

void press_reboot(void) {
	printf("Press any key to reboot.\n");
	get_key();
	vshPowerRequestColdReset();
}

int g_kernel_module, g_user_module, g_kernel2_module;

#define APP_PATH "ux0:app/MLCL00003/"

int load_helper(void) {
	int ret = 0;

	tai_module_args_t args = {0};
	args.size = sizeof(args);
	args.args = 0;
	args.argp = "";

	if ((ret = g_kernel2_module = taiLoadStartKernelModuleForUser(APP_PATH "kernel2.skprx", &args)) < 0) {
		printf("Failed to load kernel workaround: 0x%08x\n", ret);
		return -1;
	}

	if ((ret = g_kernel_module = taiLoadStartKernelModuleForUser(APP_PATH "emmc_helper.skprx", &args)) < 0) {
		printf("Failed to load kernel module: 0x%08x\n", ret);
		return -1;
	} 

	if ((ret = g_user_module = sceKernelLoadStartModule(APP_PATH "emmc_helper.suprx", 0, NULL, 0, NULL, NULL)) < 0) {
		printf("Failed to load user module: 0x%08x\n", ret);
		return -1;
	}

	return 0;
}

int stop_helper(void) {
	tai_module_args_t args = {0};
	args.size = sizeof(args);
	args.args = 0;
	args.argp = "";

	int ret = 0;
	int res = 0;

	if (g_user_module > 0) {
		ret = sceKernelStopUnloadModule(g_user_module, 0, NULL, 0, NULL, NULL);
		if (ret < 0) {
			printf("Failed to unload user module: 0x%08x\n", ret);
			return -1;
		}
	}

	if (g_kernel_module > 0) {
		ret = taiStopUnloadKernelModuleForUser(g_kernel_module, &args, NULL, &res);
		if (ret < 0) {
			printf("Failed to unload kernel module: 0x%08x\n", ret);
			return -1;
		}
	}

	if (g_kernel2_module > 0) {
		ret = taiStopUnloadKernelModuleForUser(g_kernel2_module, &args, NULL, &res);
		if (ret < 0) {
			printf("Failed to unload kernel workaround module: 0x%08x\n", ret);
			return -1;
		}
	}

	return ret;
}

int lock_system(void) {
	int ret = 0;

	printf("Locking system...\n");
	ret = sceShellUtilInitEvents(0);
	if (ret < 0) {
		printf("failed: 0x%08X\n", ret);
		return -1;
	}
	ret = sceShellUtilLock(7);
	if (ret < 0) {
		printf("failed: 0x%08X\n", ret);
		return -1;
	}
	ret = sceKernelPowerLock(0);
	if (ret < 0) {
		printf("failed: 0x%08X\n", ret);
		return -1;
	}

	return 0;
}

int unlock_system(void) {
	sceKernelPowerUnlock(0);
	sceShellUtilUnlock(7);

	return 0;
}

int do_write_recovery(void) {
	int ret = 0;
	printf("Updating recovery configuration data\n");

	if (ex("ux0:eex/recovery/rconfig.e2xp")) {
		psvDebugScreenSetFgColor(COLOR_PURPLE);
		printf(" - recovery config... ");
		ret = ensoWriteRecoveryConfig();
		if (ret < 0) {
			psvDebugScreenSetFgColor(COLOR_RED);
			printf("failed 0x%08X!\n", ret);
			psvDebugScreenSetFgColor(COLOR_WHITE);
		} else {
			psvDebugScreenSetFgColor(COLOR_GREEN);
			printf("ok!\n");
			psvDebugScreenSetFgColor(COLOR_WHITE);
		}
	}

	if (ex("ux0:eex/recovery/rblob.e2xp")) {
		psvDebugScreenSetFgColor(COLOR_PURPLE);
		printf(" - recovery blob... ");
		ret = ensoWriteRecoveryBlob();
		if (ret < 0) {
			psvDebugScreenSetFgColor(COLOR_RED);
			printf("failed 0x%08X!\n", ret);
			psvDebugScreenSetFgColor(COLOR_WHITE);
		} else {
			psvDebugScreenSetFgColor(COLOR_GREEN);
			printf("ok!\n");
			psvDebugScreenSetFgColor(COLOR_WHITE);
		}
	}

	if (ex("ux0:eex/recovery/rmbr.bin")) {
		psvDebugScreenSetFgColor(COLOR_PURPLE);
		printf(" - recovery mbr... ");
		ret = ensoWriteRecoveryMbr();
		if (ret < 0) {
			psvDebugScreenSetFgColor(COLOR_RED);
			printf("failed 0x%08X!\n", ret);
			psvDebugScreenSetFgColor(COLOR_WHITE);
		} else {
			psvDebugScreenSetFgColor(COLOR_GREEN);
			printf("ok!\n");
			psvDebugScreenSetFgColor(COLOR_WHITE);
		}
	}

	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("done!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
	return 0;
}

int do_sync_eex(void) {
	int ibootlogo = ex("os0:ex/bootlogo.raw"), ibmgr = ex("os0:" E2X_BOOTMGR_NAME), icklist = ex("os0:ex/boot_list.txt");
	int obootlogo = ex("os0:bootlogo.raw"), opatch = ex("os0:patches.e2xd"), ocbcfg = ex("os0:qsp2bootconfig.skprx");
	printf("Syncing enso_ex scripts... \n");
	
	if (ibootlogo)
		sceIoRemove("os0:ex/bootlogo.raw");
	if (ibmgr)
		sceIoRemove("os0:" E2X_BOOTMGR_NAME);
	if (icklist)
		sceIoRemove("os0:ex/boot_list.txt");
	if (obootlogo)
		sceIoRemove("os0:bootlogo.raw");
	if (opatch)
		sceIoRemove("os0:patches.e2xd");
	if (ocbcfg)
		sceIoRemove("os0:qsp2bootconfig.skprx");
	
	copyDir("ux0:eex/boot/", "os0:");
	copyDir("ux0:eex/custom/", "os0:ex/");
	
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("synced!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
	return 0;
}
	

int do_install(void) {
	int ret = 0;

	if (lock_system() < 0)
		return -1;

	printf("Checking MBR... ");
	ret = ensoCheckMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	if (ex("ur0:tai/boot_config.txt") == 0) {
		printf("Writing config... ");
		ret = ensoWriteConfig();
		if (ret < 0) {
			printf("failed\n");
			goto err;
		}
		psvDebugScreenSetFgColor(COLOR_GREEN);
		printf("ok!\n");
		psvDebugScreenSetFgColor(COLOR_WHITE);
	}
	
	printf("Copying files... ");
	sceIoMkdir("ux0:eex/", 6);
	sceIoMkdir("ux0:eex/custom/", 6);
	sceIoMkdir("ux0:eex/boot/", 6);
	sceIoMkdir("ux0:eex/recovery/", 6);
	sceIoMkdir("os0:ex/", 6);
	fcp("app0:e2xculogo.skprx", "ux0:eex/custom/e2xculogo.skprx");
	fcp("app0:e2xhencfg.skprx", "ux0:eex/custom/e2xhencfg.skprx");
	fcp("app0:" E2X_CKLDR_NAME, "ux0:eex/boot/" E2X_CKLDR_NAME);
	fcp("app0:bootlogo.raw", "ux0:eex/custom/bootlogo.raw");
	fcp("app0:boot_list.txt", "ux0:eex/custom/boot_list.txt");
	fcp("ur0:tai/boot_config.txt", "ux0:eex/boot_config.txt");
	if (ex("ur0:tai/boot_config_kitv.txt"))
		fcp("ur0:tai/boot_config_kitv.txt", "ux0:eex/boot_config_kitv.txt");
	fcp("app0:rconfig.e2xp", "ux0:eex/recovery/rconfig.e2xp");
	fcp("app0:rblob.e2xp", "ux0:eex/recovery/rblob.e2xp");
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
	do_sync_eex();

	printf("Writing blocks... ");
	ret = ensoWriteBlocks();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	printf("Writing MBR... ");
	ret = ensoWriteMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	do_write_recovery();

	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("\nThe installation was completed successfully.\n");
	psvDebugScreenSetFgColor(COLOR_GREEN);

	unlock_system();
	return 0;
err:
	unlock_system();
	return -1;
}

int do_uninstall(void) {
	int ret = 0;

	printf("Checking MBR in block 1... ");
	ret = ensoCheckRealMBR();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	printf("Uninstalling MBR patch... ");
	ret = ensoUninstallMBR();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	printf("Cleaning up payload blocks... ");
	ret = ensoCleanUpBlocks();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	printf("Deleting boot config... ");
	sceIoRemove("ur0:tai/boot_config.txt");
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	return 0;
}

int do_reinstall_config(void) {
	int ret = 0;

	printf("Writing config... ");
	ret = ensoWriteConfig();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("ok!\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	return 0;
}

int check_safe_mode(void) {
	if (sceIoDevctl("ux0:", 0x3001, NULL, 0, NULL, 0) == 0x80010030) {
		return 1;
	} else {
		return 0;
	}
}

int check_henkaku(void) {
	int fd;

	if ((fd = sceIoOpen("ur0:tai/taihen.skprx", SCE_O_RDONLY, 0)) < 0) {
		return 0;
	}
	sceIoClose(fd);
	if ((fd = sceIoOpen("ur0:tai/henkaku.skprx", SCE_O_RDONLY, 0)) < 0) {
		return 0;
	}
	sceIoClose(fd);
	return 1;
}

char mmit[][64] = { " -> Install/reinstall the hack."," -> Uninstall the hack."," -> Fix boot configuration."," -> Synchronize enso_ex scripts."," -> Update the enso_ex recovery."," -> Exit" };
int sel = 0;
int optct = 6;

void smenu(){
	psvDebugScreenClear(COLOR_BLACK);
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("                        enso_ex v5.0                             \n");
	printf("                         By SKGleba                              \n");
	psvDebugScreenSetFgColor(COLOR_RED);
	for(int i = 0; i < optct; i++){
		if(sel==i){
			psvDebugScreenSetFgColor(COLOR_GREEN);
		}
		printf("%s\n", mmit[i]);
		psvDebugScreenSetFgColor(COLOR_RED);
	}
	printf("\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	int should_reboot = 0;
	int ret = 0;

	psvDebugScreenInit();

	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("Built On: %s by SKGleba\n\n", BUILD_DATE);

	psvDebugScreenSetFgColor(COLOR_RED);
	if (check_safe_mode()) {
		printf("Please disable HENkaku Safe Mode\n\n");
		press_exit();
	}

	if (!check_henkaku()) {
		printf("Your HENkaku version is too old!\n\n");
		press_exit();
	}
	
	printf("This software will make PERMANENT modifications to your Vita\nIf anything goes wrong, there is NO RECOVERY.\n\n");
	
	psvDebugScreenSetFgColor(COLOR_GREEN);
	printf("\n\n -> I understood, continue.\n\n");
	psvDebugScreenSetFgColor(COLOR_WHITE);

	if (get_key() != SCE_CTRL_CROSS) {
		press_exit();
	}

	ret = load_helper();
	if (ret < 0)
		goto cleanup;
	
switch_opts:
	smenu();
	sceKernelDelayThread(0.3 * 1000 * 1000);
	switch (get_key()) {
		case SCE_CTRL_CROSS:
			if (sel == 0)
				ret = do_install();
			else if (sel == 1)
				ret = do_uninstall();
			else if (sel == 2)
				ret = do_reinstall_config();
			else if (sel == 3)
				ret = do_sync_eex();
			else if (sel == 4)
				ret = do_write_recovery();
			should_reboot = (sel == 5) ? 0 : 1;
			break;
		case SCE_CTRL_UP:
			if(sel!=0){
				sel--;
			}
			goto switch_opts;
		case SCE_CTRL_DOWN:
			if(sel+1<optct){
				sel++;
			}
			goto switch_opts;
		case SCE_CTRL_CIRCLE:
			break;
		default:
			goto switch_opts;
	}

	if (ret < 0) {
		psvDebugScreenSetFgColor(COLOR_RED);
		printf("\nAn error has occurred.\n\n");
		psvDebugScreenSetFgColor(COLOR_WHITE);
		printf("The log file can be found at ux0:data/enso.log\n\n");
		should_reboot = 0;
	}

cleanup:
	stop_helper();

	if (should_reboot) {
		press_reboot();
	} else {
		press_exit();
	}

	return 0;
}
