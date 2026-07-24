/* ex_defs -- custom enso_ex defs, structs, keybinds, etc
 *
 * Copyright (C) 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef __EX_DEFS_H__
#define __EX_DEFS_H__

#define E2X_MAGIC 'E2X5' // enso_ex v5 magic, checked by the recovery script

#define E2X_RECOVERY_GCSD CTRL_SELECT // run sd2vita recovery
#define E2X_RECOVERY_EMMC CTRL_START // run emmc recovery
#define E2X_RECOVERY_NOENT CTRL_TRIANGLE // ack that gcsd recovery could not read gcsd
#define E2X_RECOVERY_UNKSD CTRL_CIRCLE // ack that gcsd recovery found unknown data on gcsd
#define E2X_RECOVERY_RETERR CTRL_CROSS // // ack that gcsd recovery returned !0
#define E2X_RECOVERY_SECOND (CTRL_LEFT | CTRL_POWER | CTRL_SELECT | CTRL_SQUARE | CTRL_TRIANGLE | CTRL_CIRCLE)

#define E2X_EXE_RET_NORESIDENT 1 // exe memblock can be freed

#define E2X_IPATCHES_SKIP CTRL_VOLDOWN // skip CKLDR and BOOTMGR
#define E2X_BOOTMGR_NAME "bootmgr.e2xp" // bootmgr filename found in os0:
#define E2X_CKLDR_NAME "e2x_ckldr.skprx" // ckldr filename found in os0:
#define E2X_MAX_EPATCHES_N 32 // [CKLDR] max amount of custom boot plugins
#define E2X_EPATCHES_SKIP CTRL_VOLUP // [CKLDR] skip custom boot plugins
#define E2X_USE_BBCONFIG CTRL_SQUARE // [HEN] use ux0:eex/boot_config.txt instead of the one in ur0:tai
#define E2X_EPATCHES_DIR "os0:ex/" // [CKLDR] directory for custom boot plugins

#define E2X_BOOTAREA_LOCK_KEY 'CGB5' // [SDIF HOOK] this key lets the caller set the bootarea read-only flag
#define E2X_READ_REAL_MBR_KEY 'GRB5' // [SDIF HOOK] this key lets the caller read the real MBR
#define E2X_BOOTAREA_LOCK_CG_ACK 0xAA16 // [SDIF HOOK] confirm change of the bootarea read-only flag

#define E2X_RECOVERY_MBR_OFFSET 3 // secondary/recovery emuMBR
#define E2X_RCONF_OFFSET 4 // recovery configuration/bootstrap blob offset
#define E2X_RBLOB_OFFSET 0x30

#define E2X_BOOTMGR_PADDR 0x51f00000
#define E2X_BOOTMGR_SIZE 0xc0000
#define E2X_RBLOB_PADDR (E2X_BOOTMGR_PADDR + E2X_BOOTMGR_SIZE)
#define E2X_RBLOB_SIZE 0x39000
#define E2X_RCONF_PADDR (E2X_RBLOB_PADDR + E2X_RBLOB_SIZE)
#define E2X_RCONF_SIZE 0x200
#define E2X_CKLDR_CLIST_PADDR (E2X_RCONF_PADDR + E2X_RCONF_SIZE)
#define E2X_CKLDR_CLIST_SIZE (E2X_MAX_EPATCHES_N * 48) // approx 48 bytes per string

#define E2X_NCONF_BYTE 4
#define E2X_NCONF_NMASK 0b11000011
enum E2X_NCONF_FLAGS {
  E2X_NCONF_FLAG_SDECOND = 2, // run s2 from gc-sd
  E2X_NCONF_FLAG_NIPATCHES, // disable ipatches (ckldr patches)
  E2X_NCONF_FLAG_RGCSD, // force GC-SD recovery
  E2X_NCONF_FLAG_REMMC // force eMMC recovery
};
#define E2X_NCONF_CHK(_p, _f) (((_p)->flags.nvs[E2X_NCONF_BYTE] & ((1 << (E2X_NCONF_FLAG_##_f)) | E2X_NCONF_NMASK)) == (1 << (E2X_NCONF_FLAG_##_f)))

// expected MBR sector in external RAW recovery mode
typedef struct RecoveryBlockStruct {
  uint32_t magic; // expected enso_ex magic
  uint32_t offset; // recovery offset inside recovery sector, in bytes, |=1 for thumb
} RecoveryBlockStruct;

// enso_ex exports
typedef struct ex_ports_struct {
  char* module_dir;
  uint32_t ctrl;
  void* nskbl_exports_start;
  void* kbl_param;
  int (*get_file)(char* file_path, void* buf, uint32_t read_size, uint32_t offset);
  void* (*memset)(void* dst, int ch, int sz);
  void* (*memcpy)(void* dst, const void* src, int sz);
  void* (*get_obj_for_uid)(int uid);
  int (*alloc_memblock)(const char* name, int type, int size, void* opt);
  int (*get_memblock)(int32_t uid, void** basep);
  int (*free_memblock)(int32_t uid);
  int *protect_boot;
  int (*init_os0)(uint32_t mbr_off, unsigned int* ctx, int is_scembr);
  int (*printf)(const char* fmt, ...);
} ex_ports_struct;

// hooked get_hwcfg(array) exit array
typedef struct patchedHwcfgStruct {
  union {
    uint32_t get_ex_ports;
    uint8_t hardware_config[0x10];
    ex_ports_struct ex_ports;
  };
} patchedHwcfgStruct;

#endif