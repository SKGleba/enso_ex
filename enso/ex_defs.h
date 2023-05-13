// enso_ex defs & structs

#define E2X_MAGIC 'E2X5' // enso_ex v5 magic, checked by the recovery script
#define E2X_RECOVERY_BLKOFF 0 // legacy - sd2vita raw recovery offset
#define E2X_RECOVERY_FNAME "recovery.e2xp" // sd2vita file recovery filename
#define E2X_RECOVERY_RUN CTRL_SELECT // run sd2vita recovery
#define E2X_RECOVERY_NOENT CTRL_TRIANGLE // ack that gcsd recovery could not read gcsd
#define E2X_RECOVERY_SDERR CTRL_CIRCLE // ack that gcsd recovery found unknown data on gcsd
#define E2X_RECOVERY_RETERR CTRL_CROSS // // ack that gcsd recovery returned !0

#define E2X_CHANGE_BPARAM CTRL_START // run internal recovery/set custom boot params
#define E2X_BPARAM_LOCKBAREA CTRL_TRIANGLE // make the bootloaders and enso read-only
#define E2X_BPARAM_NOCUC CTRL_CIRCLE // DONT run internal recovery/custom config @block 4
#define E2X_BPARAM_ADIOS0 CTRL_RIGHT // dont initialize os0
#define E2X_BPARAM_CUMBR CTRL_UP // use mbr from E2X_RECOVERY_MBR_OFFSET instead of ENSO_EMUMBR_OFFSET

#define E2X_IPATCHES_SKIP CTRL_VOLDOWN // skip CKLDR and BOOTMGR
#define E2X_BOOTMGR_NAME "bootmgr.e2xp" // bootmgr filename found in os0:
#define E2X_CKLDR_NAME "e2x_ckldr.skprx" // ckldr filename found in os0:
#define E2X_MAX_EPATCHES_N 15 // [CKLDR] max amount of custom boot plugins
#define E2X_EPATCHES_SKIP CTRL_VOLUP // [CKLDR] skip custom boot plugins
#define E2X_USE_BBCONFIG CTRL_SQUARE // [HEN] use ux0:eex/boot_config.txt instead of the one in ur0:tai

#define E2X_BOOTAREA_LOCK_KEY 'CGB5' // [SDIF HOOK] this key lets the caller set the bootarea read-only flag
#define E2X_READ_REAL_MBR_KEY 'GRB5' // [SDIF HOOK] this key lets the caller read the real MBR
#define E2X_BOOTAREA_LOCK_CG_ACK 0xAA16 // [SDIF HOOK] confirm change of the bootarea read-only flag

#define E2X_RECOVERY_MBR_OFFSET 3 // secondary/recovery emuMBR

#define E2X_RCONF_OFFSET 4 // recovery configuration/bootstrap blob offset

// expected MBR sector in external recovery mode
typedef struct RecoveryBlockStruct {
  uint32_t magic;
  uint8_t flags[4];
  uint32_t blkoff;
  uint32_t blksz;
  char data[0x200 - 0x10];
} __attribute__((packed)) RecoveryBlockStruct;

// load_exe() mode arg
enum E2X_LOAD_EXE_MODES {
  E2X_LX_NO_XREMAP = 1, // dont remap to RX after alloc
  E2X_LX_BLK_SAUCE, // source is a block device
  E2X_LX_NO_CCACHE = 4 // dont clean dcache or flush icache
};

// enso_ex exports
typedef struct ex_ports_struct {
  char* module_dir;
  uint32_t ctrl;
  void* nskbl_exports_start;
  void* kbl_param;
  void* (*load_exe)(void* source, char* memblock_name, uint32_t offset, uint32_t size, int flags, int* ret_memblock_id);
  int (*get_file)(char* file_path, void* buf, uint32_t read_size, uint32_t offset);
  void* (*memset)(void* dst, int ch, int sz);
  void* (*memcpy)(void* dst, const void* src, int sz);
  void* (*get_obj_for_uid)(int uid);
  int (*alloc_memblock)(const char* name, int type, int size, void* opt);
  int (*get_memblock)(int32_t uid, void** basep);
  int (*free_memblock)(int32_t uid);
} __attribute__((packed)) ex_ports_struct;

// hooked get_hwcfg(array) exit array
typedef struct patchedHwcfgStruct {
  union {
    uint32_t get_ex_ports;
    uint8_t hardware_config[0x10];
    ex_ports_struct ex_ports;
  };
} __attribute__((packed)) patchedHwcfgStruct;