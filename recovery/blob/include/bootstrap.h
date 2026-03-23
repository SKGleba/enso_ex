#ifndef __BOOTSTRAP_H__
#define __BOOTSTRAP_H__

#include "../../../core/ex_defs.h"

enum BOOTSTRAP_MODES {
    BOOTSTRAP_MODE_EMMC = 0,
    BOOTSTRAP_MODE_GCSD,
    BOOTSTRAP_MODE_BOOTMGR,
    BOOTSTRAP_MODE_LIB
};

struct stage2_options {
    int recovery_mbr;      // Use recovery MBR
    int stage3_recovery;   // Enter stage 3 recovery
    int vanilla_boot;      // Boot vanilla OS
    int protect_bootarea;  // Protect boot area from writes
    int gcsd_mode;         // 0: disable, 1: sd0, 2: os0, 3: init
    int bootpatch;         // Run the boot patch cfg
};

// MUST be copied in
struct eex_param_s {
    enum BOOTSTRAP_MODES boot_mode;
    int (*get_hwcfg_patched)(patchedHwcfgStruct* dst);
    struct stage2_options* stage2_config;
};

#endif // __BOOTSTRAP_H__