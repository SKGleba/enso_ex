#ifndef __BOOTSTRAP_H__
#define __BOOTSTRAP_H__

#include "../../../core/ex_defs.h"

enum BOOTSTRAP_MODES {
    BOOTSTRAP_MODE_EMMC = 0,
    BOOTSTRAP_MODE_GCSD,
    BOOTSTRAP_MODE_BOOTMGR
};

// MUST be copied in
struct eex_param_s {
    enum BOOTSTRAP_MODES boot_mode;
    int (*get_hwcfg_patched)(patchedHwcfgStruct* dst);
};

#endif // __BOOTSTRAP_H__