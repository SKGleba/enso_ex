/* kernel.c -- enso installer
 *
 * Copyright (C) 2017 molecule, 2018-2022 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
 
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>

#include <taihen.h>

#include "enso.h"
#include "fatcheck.h"

#define printf(str, x...) do { printf_file("%s:%d: " str, __PRETTY_FUNCTION__, __LINE__, ## x); } while (0)
#define ARRAYSIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

int ksceSblAimgrIsDolce(void);
uint32_t crc32(uint32_t crc, const void* buf, size_t size);

void* memblock_va = NULL;
static int emmc = 0, memblock_id = 0, kitv_config = 0;

enum {
	BLOCK_SIZE = 0x200,
	OFF_PARTITION_TABLE = 0,
	OFF_REAL_PARTITION_TABLE = 1 * BLOCK_SIZE,
	OFF_FAKE_OS0 = 2 * BLOCK_SIZE,
	FAT_BIN_SIZE = 0x6000, // NOTE: first 0x400 bytes are not written
	FAT_BIN_USEFUL_SIZE = 0x6000 - 0x400,

	RBLOB_SIZE = 0x39000,

	OS0_SIZE = 0x3820 * BLOCK_SIZE,

	SL_CRC_NEW = 0xDB02B893
};

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
	char unk0[0x8];
	uint32_t loader_start;
	uint32_t loader_size;
	uint32_t act_sbls_off;
	uint32_t sbls_off[2];
	uint32_t act_os_off;
	char unk1[0x8];
	partition_t partitions[0x10];
	char unk2[0x5e];
	char unk3[0x10 * 4];
	uint16_t sig;
} __attribute__((packed)) master_block_t;

int printf_file(const char *format, ...) {
	char line[512] = {0};
	va_list arg;

	va_start(arg, format);
	vsnprintf(line, 512, format, arg);
	va_end(arg);

	int fd = ksceIoOpen("ux0:data/enso.log", SCE_O_WRONLY | SCE_O_APPEND | SCE_O_CREAT, 0777);
	if (fd < 0)
		return 0;
	ksceIoWrite(fd, line, strlen(line));
	ksceIoClose(fd);

	return 0;
}

const char *part_code(int code) {
	static char *codes[] = {
		"empty",
		"first_partition",
		"slb2",
		"os0",
		"vs0",
		"vd0",
		"tm0",
		"ur0",
		"ux0",
		"gro0",
		"grw0",
		"ud0",
		"sa0",
		"some_data",
		"pd0",
		"invalid"
	};
	return codes[code];
}

const char *part_type(int type) {
	if (type == 6)
		return "FAT16";
	else if (type == 7)
		return "exFAT";
	else if (type == 0xDA)
		return "raw";
	return "unknown";
}

const char *device = "sdstor0:int-lp-act-entire";

int run_on_thread(void *func) {
	int ret = 0;
	int res = 0;
	int uid = 0;

	ret = uid = ksceKernelCreateThread("run_on_thread", func, 64, 0x1000, 0, 0, 0);

	if (ret < 0) {
		printf("failed to create a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if ((ret = ksceKernelStartThread(uid, 0, NULL)) < 0) {
		printf("failed to start a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if ((ret = ksceKernelWaitThreadEnd(uid, &res, NULL)) < 0) {
		printf("failed to wait a thread: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	ret = res;

cleanup:
	if (uid > 0)
		ksceKernelDeleteThread(uid);

	return ret;
}

int find_active_os0(master_block_t *master) {
	int active_os0 = -1;

	for (size_t i = 0; i < ARRAYSIZE(master->partitions); ++i) {
		partition_t *p = &master->partitions[i];
		printf("Partition %d, code=%s, type=%s, active=%d, off=0x%08x, sz=0x%08x, flags=0x%08x, unk=0x%08x\n",
			i, part_code(p->code), part_type(p->type), p->active, p->off, p->sz, p->flags, p->unk);
		if (p->active == 1 && p->code == 3)
			active_os0 = i;
	}

	return active_os0;
}

int is_mbr(void *data) {
	master_block_t *master = data;
	if (memcmp(master->magic, "Sony Computer Entertainment Inc.", 0x20) != 0)
		return 0;
	if (master->sig != 0xAA55)
		return 0;
	return 1;
}

int is_empty(void *data) {
	uint8_t *buf = data;
	for (int i = 0; i < BLOCK_SIZE; ++i)
		if (buf[i] != 0xAA)
			return 0;
	return 1;
}

int dump_blocks(void) {
	int wfd = 0;
	int fd = 0;
	int ret = 0;

	printf("dumping blocks to %s\n", BLOCKS_OUTPUT);

	ret = wfd = ksceIoOpen(BLOCKS_OUTPUT, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (ret < 0) {
		printf("failed to open %s for write: 0x%08x\n", BLOCKS_OUTPUT, ret);
		ret = -1;
		goto cleanup;
	}

	ret = fd = ksceIoOpen(device, SCE_O_RDONLY, 0777);
	if (ret < 0) {
		printf("failed to open the device: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	for (int i = 0; i < FAT_BIN_SIZE / BLOCK_SIZE; ++i) {
		static char buffer[BLOCK_SIZE];
		if ((ret = ksceIoRead(fd, buffer, sizeof(buffer))) != sizeof(buffer)) {
			printf("failed to read block %d: 0x%08x\n", i, ret);
			ret = -1;
			goto cleanup;
		}
		if ((ret = ksceIoWrite(wfd, buffer, sizeof(buffer))) != sizeof(buffer)) {
			printf("failed to write block %d: 0x%08x\n", i, ret);
			ret = -1;
			goto cleanup;
		}
	}

	ret = 0;
	printf("copied successfully\n");

cleanup:
	if (wfd > 0)
		ksceIoClose(wfd);
	if (fd > 0)
		ksceIoClose(fd);

	return ret;
}

int check_blocks(void) {
	int ret = 0;
	int fd = 0;

	static master_block_t master;

	printf("checking blocks\n");

	ret = fd = ksceIoOpen(device, SCE_O_RDONLY, 0777);
	if (ret < 0) {
		printf("failed to open the device: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	// check that block 1 is MBR
	if ((ret = ksceIoLseek(fd, OFF_REAL_PARTITION_TABLE, SCE_SEEK_SET)) != OFF_REAL_PARTITION_TABLE) {
		printf("failed to seek the device to real mbr: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if ((ret = ksceIoRead(fd, &master, sizeof(master))) != sizeof(master)) {
		printf("failed to read the real mbr block: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}
	if (is_mbr(&master)) {
		// if it is, check that the rest of the blocks match a known crc32 value
		if ((ret = ksceIoLseek(fd, OFF_FAKE_OS0, SCE_SEEK_SET)) != OFF_FAKE_OS0) {
			printf("failed to seek the device to fake os0: 0x%08x\n", ret);
			ret = -1;
			goto cleanup;
		}

		uint32_t crc = 0;
		for (int i = 0; i < FAT_BIN_USEFUL_SIZE / BLOCK_SIZE; ++i) {
			static char buffer[BLOCK_SIZE];
			if ((ret = ksceIoRead(fd, buffer, sizeof(buffer))) != sizeof(buffer)) {
				printf("failed to read a block: 0x%08x\n", ret);
				ret = -1;
				goto cleanup;
			}
			crc = crc32(crc, buffer, sizeof(buffer));
		}
		printf("crc32[2; 48] = 0x%08x\n", crc);
		ret = E_PREVIOUS_INSTALL;
	} else {
		// otherwise just check that the data's empty, including real mbr
		if ((ret = ksceIoLseek(fd, OFF_REAL_PARTITION_TABLE, SCE_SEEK_SET)) != OFF_REAL_PARTITION_TABLE) {
			printf("failed to seek the device to real mbr block (2): 0x%08x\n", ret);
			ret = -1;
			goto cleanup;
		}

		// -1 because block 0 in fat.bin is fake MBR
		for (int i = 0; i < FAT_BIN_SIZE / 0x200 - 1; ++i) {
			static char buffer[BLOCK_SIZE];
			if ((ret = ksceIoRead(fd, buffer, sizeof(buffer))) != sizeof(buffer)) {
				printf("failed to read a block (2): 0x%08x\n", ret);
				ret = -1;
				goto cleanup;
			}
		}

		// all blocks checked, all good
		ret = 0;
	}

cleanup:
	if (fd > 0)
		ksceIoClose(fd);

	return ret;
}

int k_ensoCheckBlocks() {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	ret = run_on_thread(check_blocks);
	EXIT_SYSCALL(state);

	return ret;
}

int write_config() {
	int pstv = 0;
	int uid = 0;
	int ret = 0;
	int fd = 0;
	SceKernelModuleInfo info = {0};
	char *pos = NULL;
	int len = 0;

	printf("write_config\n");

	ksceIoMkdir("ur0:tai", 0777); // make directory if it does not exist

	pstv = kitv_config ? 1 : ksceSblAimgrIsGenuineDolce();
	printf("writing config for %s\n", pstv ? "PSTV" : "PS Vita");

	ret = uid = ksceKernelLoadModule(pstv ? "os0:psp2config_dolce.skprx" : "os0:psp2config_vita.skprx", 0, NULL);
	if (ret < 0) {
		printf("failed to load psp2config module: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	info.size = sizeof(info);
	ret = ksceKernelGetModuleInfo(KERNEL_PID, uid, &info);
	if (ret < 0) {
		printf("failed to get module info: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	static char config[0x1000];

	if (info.segments[0].memsz >= sizeof(config)) {
		printf("config does not fit, size=0x%08x\n", info.segments[0].memsz);
		ret = -1;
		goto cleanup;
	}

	memcpy(config, (char*)info.segments[0].vaddr + 0xD4, info.segments[0].memsz - 0xD4);

	if (memcmp(config, "#\n# PSP2", 8) != 0) {
		printf("config is corrupt\n");
		ret = -1;
		goto cleanup;
	}

	ret = fd = ksceIoOpen(kitv_config ? "ur0:tai/boot_config_kitv.txt" : "ur0:tai/boot_config.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (ret < 0) {
		printf("failed to open ur0:tai/boot_config.txt for write: 0x%08x\n", ret);
		ret = -1;
		goto cleanup;
	}

	pos = strstr(config, "\n- appspawn vs0:vsh/shell/shell.self");
	if (!pos) {
		printf("failed to patch config: cannot locate appspawn line\n");
		ret = -1;
		goto cleanup;
	}

	// write first part: warning message
	const char *patch1 = 
		"# WARNING: DO NOT EDIT THIS FILE. IF YOU JUST WANT TO RUN A PLUGIN ON BOOT,\n"
		"# EDIT ux0:tai/config.txt INSTEAD. IF YOU BREAK THIS FILE, YOUR VITA WILL NO\n"
		"# LONGER BOOT. IF THAT HAPPENS, YOU CAN ENTER SAFE MODE AND RESET ALL SETTINGS\n"
		"# TO RESET THIS FILE. THIS FILE IS UNIQUE TO EACH VITA MODEL. DO NOT BLINDLY\n"
		"# USE SOMEONE ELSE'S CONFIG.\n";
	len = strlen(patch1);
	if ((ret = ksceIoWrite(fd, patch1, len)) != len) {
		printf("failed to write config 1st part: wrote 0x%08x expected 0x%08x\n", ret, len);
		ret = -1;
		goto cleanup;
	}

	// write second part: everything before appspawn
	len = pos - config;
	if ((ret = ksceIoWrite(fd, config, len)) != len) {
		printf("failed to write config 2nd part: wrote 0x%08x expected 0x%08x\n", ret, len);
		ret = -1;
		goto cleanup;
	}

	// write 3rd part: patch: load taihen and henkaku
	const char *patch2 = "\n- load\tur0:tai/taihen.skprx\n- load\tur0:tai/henkaku.skprx\n";
	len = strlen(patch2);
	if ((ret = ksceIoWrite(fd, patch2, len)) != len) {
		printf("failed to write config 3rd part: wrote 0x%08x expected 0x%08x\n", ret, len);
		ret = -1;
		goto cleanup;
	}

	// write 4th part: rest of config
	len = strlen(pos);
	if ((ret = ksceIoWrite(fd, pos, len)) != len) {
		printf("failed to write config 4th part: wrote 0x%08x expected 0x%08x\n", ret, len);
		ret = -1;
		goto cleanup;
	}

	ret = 0;

cleanup:
	if (fd > 0)
		ksceIoClose(fd);

	if (uid > 0)
		ksceKernelUnloadModule(uid, 0, NULL);

	return ret;
}

int k_ensoWriteConfig() {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	ret = run_on_thread(write_config);
	if (!ret && (ksceSblAimgrIsTool() || ksceSblAimgrIsTest())) {
		kitv_config = 1;
		ret = run_on_thread(write_config);
		kitv_config = 0;
	}
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoWriteRecoveryMbr(void) {
	int ret = 0;
	int state = 0;
	int fd = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, BLOCK_SIZE);
	ret = fd = ksceIoOpen("ux0:eex/recovery/rmbr.bin", SCE_O_RDONLY, 0);
	if (fd >= 0) {
		ret = ksceIoRead(fd, memblock_va, BLOCK_SIZE);
		ksceIoClose(fd);
		if (*(uint32_t*)memblock_va) {
			ret = ksceSdifWriteSectorMmc(emmc, 3, memblock_va, 1);
			if (ret < 0)
				printf("failed to write mbr3\n");
		} else {
			printf("failed to read mbr3 or mbr3 empty: 0x%08X\n", ret);
			ret = -1;
		}
	} else
		printf("failed to open rmbr.bin for read: 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoWriteRecoveryConfig(void) {
	int ret = 0;
	int state = 0;
	int fd = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, BLOCK_SIZE);
	ret = fd = ksceIoOpen("ux0:eex/recovery/rconfig.e2xp", SCE_O_RDONLY, 0);
	if (fd >= 0) {
		ret = ksceIoRead(fd, memblock_va, BLOCK_SIZE);
		ksceIoClose(fd);
		if (*(uint32_t*)memblock_va) {
			ret = ksceSdifWriteSectorMmc(emmc, 4, memblock_va, 1);
			if (ret < 0)
				printf("failed to write config\n");
		} else {
			printf("failed to read config or config empty: 0x%08X\n", ret);
			ret = -1;
		}
	} else
		printf("failed to open rconfig.e2xp for read: 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoWriteRecoveryBlob(void) {
	int ret = 0;
	int state = 0;
	int fd = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, RBLOB_SIZE);
	ret = fd = ksceIoOpen("ux0:eex/recovery/rblob.e2xp", SCE_O_RDONLY, 0);
	if (fd >= 0) {
		ret = ksceIoRead(fd, memblock_va, RBLOB_SIZE);
		ksceIoClose(fd);
		if (*(uint32_t*)memblock_va) {
			ret = ksceSdifWriteSectorMmc(emmc, 0x30, memblock_va, RBLOB_SIZE / BLOCK_SIZE);
			if (ret < 0)
				printf("failed to write blob\n");
		} else {
			printf("failed to read blob or blob empty: 0x%08X\n", ret);
			ret = -1;
		}
	} else
		printf("failed to open rblob.e2xp for read: 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoWriteBlocks(void) {
	int ret = 0;
	int state = 0;
	int fd = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, FAT_BIN_SIZE);
	ret = fd = ksceIoOpen("ux0:app/MLCL00003/fat.bin", SCE_O_RDONLY, 0);
	if (fd >= 0) {
		ret = ksceIoRead(fd, memblock_va, FAT_BIN_SIZE);
		ksceIoClose(fd);
		if (ret == FAT_BIN_SIZE) {
			if (crc32(0, memblock_va, FAT_BIN_SIZE) == FATCHECK) {
				ret = ksceSdifWriteSectorMmc(emmc, 2, memblock_va + (2 * BLOCK_SIZE), (FAT_BIN_SIZE / BLOCK_SIZE) - 2);
				if (ret < 0)
					printf("failed to write enso_ex\n");
			} else {
				printf("crc doesnt match\n");
				ret = -1;
			}
		} else {
			printf("failed to read fat.bin: 0x%08X\n", ret);
			ret = -1;
		}
	} else
		printf("failed to open fat.bin for read: 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoWriteMBR(void) {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, BLOCK_SIZE);
	ret = ksceSdifReadSectorMmc(emmc, 0, memblock_va, 1);
	if (ret >= 0) {
		master_block_t* master = memblock_va;
		if (is_mbr(master)) {
			int active_os0 = find_active_os0(master);
			if (active_os0 != -1 && master->partitions[active_os0].off != 2) {
				ret = ksceSdifWriteSectorMmc(emmc, 1, memblock_va, 1);
				if (ret >= 0) {
					master->partitions[active_os0].off = 2;
					ret = ksceSdifWriteSectorMmc(emmc, 0, memblock_va, 1);
					if (ret < 0)
						printf("failed to write MBR\n");
				} else
					printf("failed to write emuMBR\n");
			} else {
				printf("failed to find active os0\n");
				ret = -1;
			}
		} else {
			printf("error: real master block is not MBR\n");
			printf("this really shouldn't happen...\n");
			ret = -1;
		}
	} else
		printf("failed to read mbr 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoCheckRealMBR(void) {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, BLOCK_SIZE);
	ret = ksceSdifReadSectorMmc(emmc, 0, memblock_va, 1);
	if (ret >= 0) {
		master_block_t* master = memblock_va;
		if (!is_mbr(master)) {
			printf("error: real master block is not MBR\n");
			printf("this really shouldn't happen...\n");
			ret = -1;
		}
	} else
		printf("failed to read mbr 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret; 
}

int k_ensoCheckMBR(void) {
	return k_ensoCheckRealMBR();
}

int k_ensoUninstallMBR(void) {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0, BLOCK_SIZE);
	ret = ksceSdifReadSectorMmc(emmc, 0, memblock_va, 1);
	if (ret >= 0) {
		master_block_t* master = memblock_va;
		if (is_mbr(master)) {
			ret = ksceSdifWriteSectorMmc(emmc, 0, memblock_va, 1);
			if (ret < 0)
				printf("failed to write mbr 0x%08X\n", ret);
		} else {
			printf("error: real master block is not MBR\n");
			printf("this really shouldn't happen...\n");
			ret = -1;
		}
	} else
		printf("failed to read mbr 0x%08X\n", ret);

	EXIT_SYSCALL(state);

	return ret;
}

int k_ensoCleanUpBlocks(void) {
	int ret = 0;
	int state = 0;

	ENTER_SYSCALL(state);
	memset(memblock_va, 0xAA, FAT_BIN_SIZE);
	ret = ksceSdifWriteSectorMmc(emmc, 1, memblock_va, (FAT_BIN_SIZE / BLOCK_SIZE) - 1);
	if (ret < 0)
		printf("failed to clean blocks 0x%08X\n", ret);
	EXIT_SYSCALL(state);

	return ret;
}

int module_start(int args, void *argv) {
	(void)args;
	(void)argv;
	emmc = ksceSdifGetSdContextPartValidateMmc(0);
	if (!emmc)
		return SCE_KERNEL_START_FAILED;

	uint8_t buf[0x200];
	master_block_t *master = buf;
	master->loader_start = 0;
	int ret = ksceSdifReadSectorMmc(emmc, 0, master, 1);
	if (ret < 0 || !master->loader_start)
		return SCE_KERNEL_START_FAILED;
	ret = ksceSdifReadSectorMmc(emmc, master->loader_start, buf, 1);
	if (ret < 0 || (crc32(0, buf + 0x40, 0x10) != SL_CRC_NEW))
		return SCE_KERNEL_START_FAILED;

	memblock_id = ksceKernelAllocMemBlock("eex_i_mb", 0x10000000 | 0xC00000 | 0x8000 | 0x6, 0x2000 * BLOCK_SIZE, NULL);
	if (memblock_id < 0)
		return SCE_KERNEL_START_FAILED;

	ksceKernelGetMemBlockBase(memblock_id, (void**)&memblock_va);
	if (!memblock_va)
		return SCE_KERNEL_START_FAILED;

	printf("enso kernel module started\n");

	return SCE_KERNEL_START_SUCCESS;
}
void _start() __attribute__ ((weak, alias ("module_start")));

int module_stop() {
	printf("enso kernel module stopped\n");

	return SCE_KERNEL_STOP_SUCCESS;
}
