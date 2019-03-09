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
#include <taihen.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "debug_screen.h"
#include "enso.h"
#include "version.h"
#include "sha256.h"

#define printf psvDebugScreenPrintf
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

int _vshSblAimgrGetConsoleId(char cid[32]);
int sceSblSsUpdateMgrSetBootMode(int x);
int vshPowerRequestColdReset(void);

int compat_mode = 0;

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

const char check_cid[16] = BUILD_CID;

int fcp(const char *from, const char *to) {
	long psz;
	FILE *fp = fopen(from,"rb");

	fseek(fp, 0, SEEK_END);
	psz = ftell(fp);
	rewind(fp);

	char* pbf = (char*) malloc(sizeof(char) * psz);
	fread(pbf, sizeof(char), (size_t)psz, fp);

	FILE *pFile = fopen(to, "wb");
	
	for (int i = 0; i < psz; ++i) {
			fputc(pbf[i], pFile);
	}
   
	fclose(fp);
	fclose(pFile);
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

int ex(const char *fname) {
    FILE *file;
    if ((file = fopen(fname, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int g_kernel_module, g_user_module, g_kernel2_module, fwver;

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
	if (ex("ur0:temp/365.t") == 1) {
		fwver = 69;
		sceIoRemove("ur0:temp/365.t");
		if ((ret = g_kernel_module = taiLoadStartKernelModuleForUser(APP_PATH "emmc_helper_365.skprx", &args)) < 0) {
			printf("Failed to load kernel module: 0x%08x\n", ret);
			return -1;
		} 
	} else if (ex("ur0:temp/360.t") == 1) {
		fwver = 34;
		sceIoRemove("ur0:temp/360.t");
		if ((ret = g_kernel_module = taiLoadStartKernelModuleForUser(APP_PATH "emmc_helper_360.skprx", &args)) < 0) {
			printf("Failed to load kernel module: 0x%08x\n", ret);
			return -1;
		}
	} else {
		printf("Failed to get fw version\n", ret);
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

int do_install_compat(void) {
	int ret = 0;

	if (lock_system() < 0)
		return -1;

	printf("Checking MBR... ");
	ret = ensoCheckMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");

	printf("Checking os0... ");
	ret = ensoCheckOs0();
	if (ret < 0) {
		printf("failed\n");
		printf("\nos0 modifications detected.\nYou should reinstall the firmware and try again.\n");

		printf("Press X to continue, any other key to exit.\n");
		if (get_key() != SCE_CTRL_CROSS) {
			goto err;
		}
	}
	printf("ok!\n");
	printf("\n");

	printf("Checking for previous installation... ");
	ret = ensoCheckBlocks();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n", ret);
	if (ret == 0) {
		// all good, blocks are empty
	} else if (ret == E_PREVIOUS_INSTALL) {
		printf("Previous installation was detected and will be overwritten.\nPress X to continue, any other key to exit.\n");
		if (get_key() != SCE_CTRL_CROSS)
			goto err;
	} else if (ret == E_MBR_BUT_UNKNOWN) {
		printf("MBR was detected but installation checksum does not match.\nA dump was created at %s.\nPress X to continue, any other key to exit.\n", BLOCKS_OUTPUT);
		if (get_key() != SCE_CTRL_CROSS)
			goto err;
	} else if (ret == E_UNKNOWN_DATA) {
		printf("Unknown data was detected.\nA dump was created at %s.\nPress [] to continue, any other key to exit.\n", BLOCKS_OUTPUT);
		if (get_key() != SCE_CTRL_SQUARE)
			goto err;
	} else {
		printf("Unknown error code.\n");
		goto err;
	}
	
	printf("Copying config... ");
	fcp("ux0:eex/boot_config_u.txt", "ur0:tai/boot_config.txt");
	fcp("ux0:eex/boot_config_s.txt", "sa0:eex/boot_config.txt");
	printf("ok!\n");

	printf("Writing blocks... ");
	ret = ensoWriteBlocks();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");

	printf("Writing MBR... ");
	ret = ensoWriteMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");
	
	printf("copying taihen and henkaku... ");
	fcp("ur0:tai/taihen.skprx", "sa0:eex/taihen.skprx");
	fcp("ur0:tai/henkaku.skprx", "sa0:eex/henkaku.skprx");
	printf("done\n");
	
	printf("copying stock bootanimation... ");
	fcp("app0:banim.xgif", "ur0:tai/boot_animation.img");
	printf("done\n");
	
	printf("copying stock bootlogo... ");
	fcp("app0:blogo.xpng", "sa0:eex/boot_splash.img");
	printf("done\n");

	printf("\nThe installation was completed successfully.\n");

	unlock_system();
	return 0;
err:
	unlock_system();
	return -1;
}

int do_install(void) {
	int ret = 0;
	static char flags[4] = { 0x31, 0x30, 0x30, 0x30 };

	if (lock_system() < 0)
		return -1;

	printf("Checking MBR... ");
	ret = ensoCheckMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");

	printf("Checking os0... ");
	ret = ensoCheckOs0();
	if (ret < 0) {
		printf("failed\n");
		printf("\nos0 modifications detected.\nYou should reinstall the firmware and try again.\n");

		printf("Press X to continue, any other key to exit.\n");
		if (get_key() != SCE_CTRL_CROSS) {
			goto err;
		}
	}
	printf("ok!\n");
	printf("\n");

	printf("Checking for previous installation... ");
	ret = ensoCheckBlocks();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n", ret);
	if (ret == 0) {
		// all good, blocks are empty
	} else if (ret == E_PREVIOUS_INSTALL) {
		printf("Previous installation was detected and will be overwritten.\nPress X to continue, any other key to exit.\n");
		if (get_key() != SCE_CTRL_CROSS)
			goto err;
	} else if (ret == E_MBR_BUT_UNKNOWN) {
		printf("MBR was detected but installation checksum does not match.\nA dump was created at %s.\nPress X to continue, any other key to exit.\n", BLOCKS_OUTPUT);
		if (get_key() != SCE_CTRL_CROSS)
			goto err;
	} else if (ret == E_UNKNOWN_DATA) {
		printf("Unknown data was detected.\nA dump was created at %s.\nPress [] to continue, any other key to exit.\n", BLOCKS_OUTPUT);
		if (get_key() != SCE_CTRL_SQUARE)
			goto err;
	} else {
		printf("Unknown error code.\n");
		goto err;
	}
	
	printf("\nCustom option 1 - Boot Logo\n");
	printf("Options:\n");
	printf("  CROSS      Custom Boot Logo/Animation\n");
	printf("  SQUARE     'Playstation' Boot Logo\n\n");

	if (get_key() == SCE_CTRL_SQUARE) flags[0] = 0x30;
	
	printf("\nCustom option 2 - SD2VITA driver\n");
	printf("Options:\n");
	printf("  CROSS      Disabled\n");
	printf("  SQUARE     Enabled\n\n");

	if (get_key() == SCE_CTRL_SQUARE) flags[1] = 0x31;
	
	printf("\nCustom option 3 - Update process\n");
	printf("Options:\n");
	printf("  CROSS      Stock\n");
	printf("  SQUARE     Block updates\n");
	printf("  TRIANGLE   SoftUpdater (ADVANCED USERS ONLY)\n\n");

	int kii = get_key();
	
	if (kii == SCE_CTRL_SQUARE) {
		flags[3] = 0x31;
	} else if (kii == SCE_CTRL_TRIANGLE) {
		flags[2] = 0x31;
	}
	
	printf("Configuring enso... ");
	if (fwver == 34) {
		fcp("app0:fat_360.bin", "ur0:temp/slim.bin");
	} else {
		fcp("app0:fat_365.bin", "ur0:temp/slim.bin");
	}
	
	static char config[8];
	
	SceUID wfd = sceIoOpen("ur0:temp/slim.bin", SCE_O_RDWR, 0777);
	sceIoLseek(wfd, 0x2707, SCE_SEEK_SET);
	sceIoRead(wfd, config, 8);
	
	if (strncmp(config, "1000-CFG", 8) != 0) {
		printf("UNKPAYLOAD: %s\n", config);
		goto err;
	}
	
	sceIoLseek(wfd, 0, SCE_SEEK_SET);
	sceIoLseek(wfd, 0x2707, SCE_SEEK_SET);
	sceIoWrite(wfd, flags, 4);
	sceIoClose(wfd);
	printf("done\n");
	
	printf("Writing config... ");
	ret = ensoWriteConfig();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");

	printf("Writing blocks... ");
	ret = ensoWriteBlocks();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");

	printf("Writing MBR... ");
	ret = ensoWriteMBR();
	if (ret < 0) {
		printf("failed\n");
		goto err;
	}
	printf("ok!\n");
	
	ksceIoMkdir("sa0:eex", 0777);
	
	printf("copying taihen and henkaku... ");
	fcp("ur0:tai/taihen.skprx", "sa0:eex/taihen.skprx");
	fcp("ur0:tai/henkaku.skprx", "sa0:eex/henkaku.skprx");
	printf("done\n");
	
	printf("copying stock bootanimation... ");
	fcp("app0:banim.xgif", "ur0:tai/boot_animation.img");
	printf("done\n");
	
	printf("copying stock bootlogo... ");
	fcp("app0:blogo.xpng", "sa0:eex/boot_splash.img");
	printf("done\n");

	printf("\nThe installation was completed successfully.\n");
	
	sceIoRemove("ur0:temp/slim.bin");

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
	printf("ok!\n");

	printf("Uninstalling MBR patch... ");
	ret = ensoUninstallMBR();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	printf("ok!\n");

	printf("Cleaning up payload blocks... ");
	ret = ensoCleanUpBlocks();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	printf("ok!\n");

	printf("Deleting boot config... ");
	sceIoRemove("ur0:tai/boot_config.txt");
	sceIoRemove("sa0:eex/boot_config.txt");
	printf("ok!\n");

	return 0;
}

int do_reinstall_config(void) {
	int ret = 0;
	
	ksceIoMkdir("sa0:eex", 0777);

	printf("Writing config... ");
	ret = ensoWriteConfig();
	if (ret < 0) {
		printf("failed\n");
		return -1;
	}
	printf("ok!\n");

	return 0;
}

int check_build(void) {
	if (BUILD_PERSONALIZED) {
		char right_cid[16];
		char cur_cid[16];
		for (int i = 0; i < 16; i++) {
			right_cid[i] = check_cid[i] ^ 0xAA; // super leet encryption
		}
		_vshSblAimgrGetConsoleId(cur_cid);
		if (memcmp(cur_cid, right_cid, 16) == 0) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 1;
	}
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
		if ((fd = sceIoOpen("sa0:eex/taihen.skprx", SCE_O_RDONLY, 0)) < 0) {
			return 0;
		}
	}
	sceIoClose(fd);
	if ((fd = sceIoOpen("ur0:tai/henkaku.skprx", SCE_O_RDONLY, 0)) < 0) {
		if ((fd = sceIoOpen("sa0:eex/henkaku.skprx", SCE_O_RDONLY, 0)) < 0) {
			return 0;
		}
	}
	sceIoClose(fd);
	return 1;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	int should_reboot = 0;
	int ret = 0;

	psvDebugScreenInit();

	if (!check_build()) {
		return 0;
	}

	printf("Built On: %s by SKGleba ( twitter.com/skgleba )\n\n", BUILD_DATE);

	if (check_safe_mode()) {
		printf("Please disable HENkaku Safe Mode from Settings before running this installer.\n\n");
		press_exit();
	}

	if (!check_henkaku()) {
		printf("Your HENkaku version is too old! Please install R10 from https://henkaku.xyz/go/ (not the offline installer!)\n\n");
		press_exit();
	}

#if BUILD_PERSONALIZED
	printf("Please visit https://enso.henkaku.xyz/beta/ for installation instructions.\n\n");

	uint32_t sequence[] = { SCE_CTRL_CROSS, SCE_CTRL_TRIANGLE, SCE_CTRL_SQUARE, SCE_CTRL_CIRCLE };
	for (size_t i = 0; i < sizeof(sequence)/sizeof(*sequence); ++i) {
		if (get_key() != sequence[i])
			press_exit();
	}
#endif

	printf("This software will make PERMANENT modifications to your Vita. If anything goes wrong, \n"
		   "there is NO RECOVERY (not even with a hardware flasher). The creators provide this \n"
		   "tool \"as is\", without warranty of any kind, express or implied and cannot be held \n"
		   "liable for any damage done.\n\n");
	printf("Press CIRCLE to accept these terms or any other key to not accept.\n");

	if (get_key() != SCE_CTRL_CIRCLE) {
		press_exit();
	}

	ret = load_helper();
	if (ret < 0)
		goto cleanup;
	
	if (fwver == 34) {
		printf("Firmware: 3.60\n\n");
	} else if (fwver == 69) {
		printf("Firmware: 3.65\n\n");
	}
	
	printf("Options:\n\n");
	printf("  CROSS      Install/reinstall the hack.\n");
	printf("  TRIANGLE   Uninstall the hack.\n");
	printf("  SQUARE     Fix boot configuration (choose this if taiHEN isn't loading on boot).\n");
	printf("  CIRCLE     Exit without doing anything.\n\n");

again:
	switch (get_key()) {
	case SCE_CTRL_CROSS:
		ret = do_install();
		should_reboot = 1;
		break;
	case SCE_CTRL_TRIANGLE:
		ret = do_uninstall();
		should_reboot = 1;
		break;
	case SCE_CTRL_SQUARE:
		ret = do_reinstall_config();
		break;
	case SCE_CTRL_CIRCLE:
		break;
	case SCE_CTRL_START && SCE_CTRL_SELECT:
		ret = do_install_compat();
		break;
	default:
		goto again;
	}

	if (ret < 0) {
		printf("\nAn error has occurred.\n");
		printf("The log file can be found at ux0:data/enso.log\n\n");
		should_reboot = 0;
	} else {
		printf("Success.\n\n");
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
