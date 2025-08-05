/* first.c -- fixup environment and exec second payload
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>
#include "nskbl.h"
#include "enso.h"
#include "misc_bm.h"
#include "ex_defs.h"

// first payload says hello, cleans up and reads second payload from emmc
// this is because we only have 0x180 bytes for first payload
void go(void) {
    // say hello
    printf("\nenso_ex v5 | @stage1\n\n");

    // clean after us
    memset((char*)ENSO_CORRUPTED_AREA_START, 0, ENSO_CORRUPTED_AREA_SIZE);

    // restore corrupted boot args with our copy
    memcpy(boot_args, (*sysroot_ctx_ptr)->boot_args, sizeof(*boot_args));
	
    // memblock for stage2
    printf("x stage2 a");
    void* stage2 = NULL;
    int blk = sceKernelAllocMemBlock("", MEMBLOCK_TYPE_RW, SECOND_PAYLOAD_SIZE, NULL);
    sceKernelGetMemBlockBase(blk, &stage2);
    
    // read stage2 from emmc or gcsd
    printf("-r");
    if (CTRL_BUTTON_HELD(boot_args->field_CC, E2X_RECOVERY_SECOND) && *(uint32_t*)NSKBL_DEVICE_GCSD_TGT_CTX)
        read_sector_sd((int*)*(uint32_t*)NSKBL_DEVICE_GCSD_TGT_CTX, SECOND_PAYLOAD_OFFSET, stage2, SECOND_PAYLOAD_SIZE / SDIF_SECTOR_SIZE);
    else
        read_sector_mmc_direct((int*)*(uint32_t*)NSKBL_DEVICE_EMMC_TGT_CTX, SECOND_PAYLOAD_OFFSET, stage2, SECOND_PAYLOAD_SIZE / SDIF_SECTOR_SIZE);

    // rw->rx
    printf("-m");
    sceKernelRemapBlock(blk, MEMBLOCK_TYPE_RX);
    clean_dcache(stage2, SECOND_PAYLOAD_SIZE);
    flush_icache();

    // run stage2
    printf("-x\n");
    void (*stage2_start)(void *me) = (void*)(stage2 + 1);
    stage2_start(stage2);
}

__attribute__ ((section (".text.start"), naked)) void start(void)  {
    __asm__ volatile (
        "mov r0, #0\n"
        "movt r0, #0x51f0\n"
        "mov sp, r0\n"
        "b go\n"
    );
}
