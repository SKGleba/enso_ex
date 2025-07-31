#include <inttypes.h>
#include "../../core/nskbl.h"
#include "../../core/ex_defs.h"
#include "../../installer/src/enso.h"
#include "../blob/include/bootstrap.h"

#define BLOB_SIZE (RBLOB_SIZE / BLOCK_SIZE)
#define BLOB_OFFSET 2 // on GCSD its just after the bootstrap
#define BLOB_MEMLOC 0x51f00000

// recovery block, parsed by enso_ex
__attribute__((section(".text._my_info"))) const RecoveryBlockStruct _my_info = {
    E2X_MAGIC,  // expected enso_ex magic
    0,          // this recovery offset on GCSD, in sectors
    1,          // this recovery size on GCSD, in sectors
    0x11        // this recovery offset inside recovery sector, in bytes, |=1 for thumb
};

__attribute__((section(".text.start"))) int start(uint32_t get_info_va) {
    // print hello
    printf("[E2X@R] welcome to rbootstrap(0x%08X)!\n", (uint32_t)get_info_va);

    // read blob
    printf("[E2X@R] rblob[0x%08X@0x%08X] r -> 0x%08X\n", BLOB_SIZE * BLOCK_SIZE, BLOB_OFFSET * BLOCK_SIZE, BLOB_MEMLOC);
    int ret = read_sector_default_direct((int *)NSKBL_DEVICE_GCSD_CTX, BLOB_OFFSET, BLOB_SIZE, (int)BLOB_MEMLOC);
    if (ret < 0) {
        printf("[E2X@R] rblob read failed: 0x%08X\n", ret);
        return -2;
    }

    // prep args
    struct eex_param_s params = {
        .init_os0 = NULL,
        .get_hwcfg_patched = (void *)get_info_va,
        .kbl_param = (void *)boot_args,
        .disable_bootarea_update = NULL
    };

    // run blob
    printf("[E2X@R] rblob x..\n");
    int (*blob_start)(struct eex_param_s *params) = (void*)(BLOB_MEMLOC | 1);
    ret = blob_start(&params);
    printf("rblob returned 0x%08X\n", ret);

    // print bye
    printf("[E2X@R] exiting rbootstrap\n");

    return 1;
}