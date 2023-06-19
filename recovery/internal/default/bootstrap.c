#include <inttypes.h>
#include "../../../core/nskbl.h"
#include "../../../core/ex_defs.h"

#define BLOCK_SIZE 0x200
#define BLOB_SIZE (0x39000 / BLOCK_SIZE)
#define BLOB_OFFSET 0x30

void _start(uint32_t get_info_va, uint32_t init_os0_va, uint32_t load_exe_va) {
    // print hello
    printf("[E2X@R] welcome to rbootstrap(0x%08X | 0x%08X)!\n", get_info_va, init_os0_va);

    void *(*load_exe)(void* source, char* memblock_name, uint32_t offset, uint32_t size, int flags, int* ret_memblock_id) = (void*)load_exe_va;

    // alloc memblock for le blob
    printf("[E2X@R] rblob[0x%08X@0x%08X] arm..\n", BLOB_SIZE * BLOCK_SIZE, BLOB_OFFSET * BLOCK_SIZE);
    int blk = 0;
    void* blob = load_exe((int *)NSKBL_DEVICE_EMMC_CTX, "rblob", BLOB_OFFSET, BLOB_SIZE, E2X_LX_BLK_SAUCE, &blk);
    if (!blob) {
        printf("arm failed 0x%08X 0x%08X\n", blk, blob);
        return;
    }

    // run blob
    printf("rblob x..\n");
    int (*blob_start)(uint32_t get_info_va, uint32_t init_os0_va, void *block) = (void*)(blob + 1);
    int ret = blob_start(get_info_va, init_os0_va, blob);
    printf("rblob returned 0x%08X\n", ret);

    // free blob memblock if requested
    if (ret & 1)
        sceKernelFreeMemBlock(blk);

    // print bye
    printf("[E2X@R] exiting rbootstrap\n");

    return;
}