#include <inttypes.h>
#include "../../../core/nskbl.h"
#include "../../../core/ex_defs.h"
#include "../../../installer/src/enso.h"

#define BLOB_SIZE (RBLOB_SIZE / BLOCK_SIZE)
#define BLOB_OFFSET RBLOB_TARGET
#define BLOB_MEMLOC 0x51f00000

void _start(uint32_t get_info_va, uint32_t init_os0_va, uint32_t load_exe_va) {
    // print hello
    printf("[E2X@R] welcome to rbootstrap(0x%08X | 0x%08X)!\n", get_info_va, init_os0_va);

    // alloc memblock for le blob
    printf("[E2X@R] rblob[0x%08X@0x%08X] r -> 0x%08X\n", BLOB_SIZE * BLOCK_SIZE, BLOB_OFFSET * BLOCK_SIZE, BLOB_MEMLOC);
    int ret = read_sector_default_direct((int *)NSKBL_DEVICE_EMMC_CTX, BLOB_OFFSET, BLOB_SIZE, (int)BLOB_MEMLOC);
    if (ret < 0) {
        printf("[E2X@R] rblob read failed: 0x%08X\n", ret);
        return;
    }

    // run blob
    printf("[E2X@R] rblob x..\n");
    int (*blob_start)(uint32_t get_info_va, uint32_t init_os0_va, void *block) = (void*)(BLOB_MEMLOC | 1);
    ret = blob_start(get_info_va, init_os0_va, (void *)BLOB_MEMLOC);
    printf("rblob returned 0x%08X\n", ret);

    // print bye
    printf("[E2X@R] exiting rbootstrap\n");

    return;
}