
#include <inttypes.h>
#include <stddef.h>

#include "bootmgr.h"

__attribute__((section(".text.bootstart"))) int bootstart(void) {
	struct eex_param_s tmp_eex_params = {
            .boot_mode = BOOTSTRAP_MODE_LIB, // _BOOTMGR will autoinit the display and require calling deinit() on exit
            .get_hwcfg_patched = (int (*)(patchedHwcfgStruct *))(*(uint32_t*)NSKBL_EXPORTS(NSKBL_EXPORTS_GET_HWCFG_N)),
            .stage2_config = NULL
        };
    if (init(&tmp_eex_params) < 0) {
        nskbl_printf("Failed to initialize e2xr in LIB mode\n");
        return -1;
    }
    if (fmgr_get_nskbl_os0(true) != (uint32_t)-1) {
        ILOG("cmdh_lxp(%s) returned: 0x%08X\n", BOOTPATCH_TXTCFG, cmdh_lxp(BOOTPATCH_TXTCFG));
    } // no need to unmount os0, we are R/O
    deinit();
    return E2X_EXE_RET_NORESIDENT; // indicate that we can be freed
}