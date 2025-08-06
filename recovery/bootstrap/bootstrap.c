#include <inttypes.h>
#include "../../core/nskbl.h"
#include "../../core/ex_defs.h"
#include "../blob/include/bootstrap.h"

#define BLOB_SIZE (E2X_RBLOB_SIZE / SDIF_SECTOR_SIZE)
#define BLOB_OFFSET E2X_RBLOB_OFFSET
#define BLOB_MEMLOC E2X_RBLOB_PADDR

// recovery block, parsed by enso_ex
__attribute__((section(".text._my_info"))) const RecoveryBlockStruct _my_info = {
    E2X_MAGIC,  // expected enso_ex magic
    0x21        // this recovery offset inside recovery sector, in bytes, |=1 for thumb
};

__attribute__((section(".text._s2cfg"))) const struct stage2_options _s2cfg = {
    .recovery_mbr = 0,
    .stage3_recovery = 0,
    .vanilla_boot = 0,
    .protect_bootarea = 0,
    .gcsd_mode = 0,
    .reserved = 0
};

__attribute__((section(".text.start"))) int start(int *sctx, uint32_t get_info_va) {
    // print hello
    printf("[E2X@R] welcome to rbootstrap(0x%08X)!\n", sctx);

    // read blob
    printf("[E2X@R] rblob[0x%08X@0x%08X] r -> 0x%08X\n", BLOB_SIZE * SDIF_SECTOR_SIZE, BLOB_OFFSET * SDIF_SECTOR_SIZE, BLOB_MEMLOC);
    int ret = read_sector_default_direct(sctx, BLOB_OFFSET, BLOB_SIZE, (int)BLOB_MEMLOC);
    if (ret < 0) {
        printf("[E2X@R] rblob read failed: 0x%08X\n", ret);
        return ret;
    }

    // prep args
    struct eex_param_s params = {
        .boot_mode = (sctx == (int*)NSKBL_DEVICE_GCSD_CTX) ? BOOTSTRAP_MODE_GCSD : BOOTSTRAP_MODE_EMMC,
        .get_hwcfg_patched = (void *)get_info_va,
        .stage2_config = &_s2cfg,
    };

    // run blob
    printf("[E2X@R] rblob x..\n");
    int (*blob_start)(struct eex_param_s *params) = (void *)(BLOB_MEMLOC | 1);
    ret = blob_start(&params);
    printf("[E2X@R] rblob returned 0x%08X\n", ret);

    // print bye
    printf("[E2X@R] exiting rbootstrap\n");

    return ret;
}