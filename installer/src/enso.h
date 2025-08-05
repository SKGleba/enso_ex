#pragma once

// base defines
#define BLOCK_SIZE 0x200
#define OFF_PARTITION_TABLE 0                      // MBR pointing to the fake os0
#define OFF_REAL_PARTITION_TABLE (1 * BLOCK_SIZE)  // real MBR backup
#define OFF_FAKE_OS0 (2 * BLOCK_SIZE)              // fake os0 partition with our stage1

#define INSTALLER_PATH "ux0:app/MLCL00003/"
#define EEX_ADDONS_PATH "ux0:eex/"

#define INSTALLER_PREK_PATH INSTALLER_PATH "kernel2.skprx"
#define INSTALLER_MAINK_PATH INSTALLER_PATH "emmc_helper.skprx"
#define INSTALLER_MAINU_PATH INSTALLER_PATH "emmc_helper.suprx"

#define FAT_BIN_SOURCE INSTALLER_PATH "fat.bin"
#define FAT_BIN_SIZE 0x6000
#define FAT_BIN_USEFUL_START (2 * BLOCK_SIZE) // start of the useful data in fat.bin
#define FAT_BIN_USEFUL_SIZE (FAT_BIN_SIZE - FAT_BIN_USEFUL_START)  // first 2 blocks are not written
#define FAT_BIN_TARGET 0x2 // start block for the fat.bin

#define RBLOB_SOURCE "ux0:eex/recovery/rblob.e2xp"
#define RMBR_SOURCE "ux0:eex/recovery/rmbr.bin"
#define RCONFIG_SOURCE "ux0:eex/recovery/rconfig.e2xp"

#define VALID_SL_CRC 0xDB02B893  // 3.65 second_loader

#define BLOCKS_OUTPUT "ux0:data/blocks.bin"
#define LOG_OUTPUT "ux0:data/enso.log"
#define SDSTOR_TARGET_DEV "sdstor0:int-lp-act-entire" // target addr for sdstor
#define MBR_MAGIC_STR "Sony Computer Entertainment Inc."
#define MBR_MAGIC_STR_LEN 0x20
#define MBR_SECTOR_SIG 0xAA55
#define EMPTY_SECTOR_BYTE 0xAA // byte used to fill empty blocks

#define PSP2CONFIG_PSTV_PATH "os0:psp2config_dolce.skprx"
#define PSP2CONFIG_VITA_PATH "os0:psp2config_vita.skprx"
#define PSP2CONFIG_TXT_START 0xD4
#define PSP2CONFIG_TXT_MAGIC "#\n# PSP2"
#define PSP2CONFIG_TXT_MAGIC_LEN 8
#define ENSO_PSP2CONFIG_VITA_PATH "ur0:tai/boot_config.txt"
#define ENSO_PSP2CONFIG_DEVKITV_PATH "ur0:tai/boot_config_kitv.txt"
#define TAIHEN_PATH "ur0:tai/taihen.skprx"
#define HENKAKU_PATH "ur0:tai/henkaku.skprx"
#define TAIHENKAKU_PSP2CONFIG_PATCH "\n- load\t" TAIHEN_PATH "\n- load\t" HENKAKU_PATH "\n"

#define BOOTEXT_DIR EEX_ADDONS_PATH "boot/"
#define KERNEXT_DIR EEX_ADDONS_PATH "custom/"
#define RECVEXT_DIR EEX_ADDONS_PATH "recovery/"

#define LOCAL_CULOGO_PATH "e2xculogo.skprx"
#define EXT_CULOGO_PATH KERNEXT_DIR "e2xculogo.skprx"
#define LOCAL_HENCFG_PATH "e2xhencfg.skprx"
#define EXT_HENCFG_PATH KERNEXT_DIR "e2xhencfg.skprx"
#define LOCAL_CKLDR_PATH "e2x_ckldr.skprx"
#define LOCAL_BOOTLOGO_PATH "bootlogo.raw"
#define EXT_BOOTLOGO_PATH KERNEXT_DIR "bootlogo.raw"
#define LOCAL_BOOTLIST_PATH "boot_list.txt"
#define EXT_BOOTLIST_PATH KERNEXT_DIR "boot_list.txt"
#define EXT_BACKUP_PSP2CONFIG_VITA_PATH EEX_ADDONS_PATH "boot_config.txt"
#define EXT_BACKUP_PSP2CONFIG_DEVKITV_PATH EEX_ADDONS_PATH "boot_config_kitv.txt"
#define LOCAL_RCONFIG_PATH "rconfig.e2xp"
#define EXT_RCONFIG_PATH RECVEXT_DIR "rconfig.e2xp"
#define LOCAL_RBLOB_PATH "rblob.e2xp"
#define EXT_RBLOB_PATH RECVEXT_DIR "rblob.e2xp"

#define OLD_BOOTLOGO_PATH "os0:bootlogo.raw"
#define OLD_PATCHES_PATH "os0:patches.e2xd"
#define OLD_CKLDR_PATH "os0:qsp2bootconfig.skprx"

#define INSTALLER_VERSION "enso_ex v5.1"

enum {
    E_PREVIOUS_INSTALL = 1,
    E_MBR_BUT_UNKNOWN = 2,
    E_UNKNOWN_DATA = 3,
};

// user prototypes
int ensoCheckOs0(void);
int ensoCheckMBR(void);
int ensoCheckBlocks(void);
int ensoWriteConfig(void);
int ensoWriteBlocks(void);
int ensoWriteMBR(void);
int ensoCheckRealMBR(void);
int ensoUninstallMBR(void);
int ensoCleanUpBlocks(void);
int ensoWriteRecoveryConfig(void);
int ensoWriteRecoveryBlob(void);
int ensoWriteRecoveryMbr(void);

// kernel prototypes
int k_ensoCheckOs0(void);
int k_ensoCheckMBR(void);
int k_ensoCheckBlocks(void);
int k_ensoWriteConfig(void);
int k_ensoWriteBlocks(void);
int k_ensoWriteMBR(void);
int k_ensoCheckRealMBR(void);
int k_ensoUninstallMBR(void);
int k_ensoCleanUpBlocks(void);
int k_ensoWriteRecoveryConfig(void);
int k_ensoWriteRecoveryBlob(void);
int k_ensoWriteRecoveryMbr(void);

