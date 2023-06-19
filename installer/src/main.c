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

#include "../../core/ex_defs.h"

#define printf psvDebugScreenPrintf
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define CHUNK_SIZE 64 * 1024
#define hasEndSlash(path) (path[strlen(path) - 1] == '/')

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

int fcp(const char* src, const char* dst) {
	sceClibPrintf("Copying %s -> %s (file)... ", src, dst);
	int res;
	SceUID fdsrc = -1, fddst = -1;
	void* buf = NULL;

	res = fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
	if (res < 0)
		goto err;

	res = fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (res < 0)
		goto err;

	buf = memalign(4096, CHUNK_SIZE);
	if (!buf) {
		res = -1;
		goto err;
	}

	do {
		res = sceIoRead(fdsrc, buf, CHUNK_SIZE);
		if (res > 0)
			res = sceIoWrite(fddst, buf, res);
	} while (res > 0);

err:
	if (buf)
		free(buf);
	if (fddst >= 0)
		sceIoClose(fddst);
	if (fdsrc >= 0)
		sceIoClose(fdsrc);
	sceClibPrintf("%s\n", (res < 0) ? "FAILED" : "OK");
	return res;
}

int copyDir(const char* src_path, const char* dst_path) {
	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		sceClibPrintf("Copying %s -> %s (dir)\n", src_path, dst_path);
		SceIoStat stat;
		sceClibMemset(&stat, 0, sizeof(SceIoStat));
		sceIoGetstatByFd(dfd, &stat);

		stat.st_mode |= SCE_S_IWUSR;

		sceIoMkdir(dst_path, 6);

		int res = 0;

		do {
			SceIoDirent dir;
			sceClibMemset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char* new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				sceClibSnprintf(new_src_path, 1024, "%s%s%s", src_path, hasEndSlash(src_path) ? "" : "/", dir.d_name);

				char* new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
				sceClibSnprintf(new_dst_path, 1024, "%s%s%s", dst_path, hasEndSlash(dst_path) ? "" : "/", dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode))
					ret = copyDir(new_src_path, new_dst_path);
				else {
					psvDebugScreenSetFgColor(COLOR_PURPLE);
					printf("  -  %s... ", dir.d_name);
					ret = fcp(new_src_path, new_dst_path);
					if (ret < 0) {
						psvDebugScreenSetFgColor(COLOR_RED);
						printf("FAILED!\n");
					} else {
						psvDebugScreenSetFgColor(COLOR_GREEN);
						printf("ok!\n");
					}
					psvDebugScreenSetFgColor(COLOR_WHITE);
				}

				free(new_dst_path);
				free(new_src_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else
		return fcp(src_path, dst_path);

	return 1;
}

int removeDir(const char* path) {
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		sceClibPrintf("Removing %s (dir)\n", path);
		SceIoStat stat;
		sceClibMemset(&stat, 0, sizeof(SceIoStat));
		sceIoGetstatByFd(dfd, &stat);

		stat.st_mode |= SCE_S_IWUSR;

		int res = 0;

		do {
			SceIoDirent dir;
			sceClibMemset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char* new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				sceClibSnprintf(new_path, 1024, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode))
					ret = removeDir(new_path);
				else {
					sceClibPrintf("Removing %s (file)\n", new_path);
					ret = sceIoRemove(new_path);
				}

				free(new_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
		sceIoRmdir(path);
	} else if (ex(path)) {
		sceClibPrintf("Removing %s (file)\n", path);
		sceIoRemove(path);
	} else
		sceClibPrintf("Bad rmdir input: %s\n", path);

	return 1;
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
	printf("Syncing enso_ex scripts... \n");

	// old
	if (ex("os0:bootlogo.raw"))
		sceIoRemove("os0:bootlogo.raw");
	if (ex("os0:patches.e2xd"))
		sceIoRemove("os0:patches.e2xd");
	if (ex("os0:qsp2bootconfig.skprx"))
		sceIoRemove("os0:qsp2bootconfig.skprx");

	// core extensions
	if (!(ex("ux0:eex/boot/" E2X_BOOTMGR_NAME)) && ex("os0:" E2X_BOOTMGR_NAME))
		sceIoRemove("os0:" E2X_BOOTMGR_NAME);
	if (!(ex("ux0:eex/boot/" E2X_CKLDR_NAME)) && ex("os0:" E2X_CKLDR_NAME))
		sceIoRemove("os0:" E2X_CKLDR_NAME);

	// remove old plugins
	removeDir("os0:ex/");

	// copy new extensions and plugins
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

char mmit[][64] = { " -> Install/reinstall the hack"," -> Uninstall the hack"," -> Fix boot configuration"," -> Synchronize enso_ex plugins"," -> Update the enso_ex recovery"," -> Exit" };
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
