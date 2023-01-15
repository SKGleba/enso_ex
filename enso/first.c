/* first.c -- fixup environment and exec second payload
 *
 * Copyright (C) 2017 molecule, 2018-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <inttypes.h>
#include "nsbl.h"
#include "enso.h"

// first payload says hello, cleans up and reads second payload from emmc
// this is because we only have 0x180 bytes for first payload
void go(void) {
    // say hello
    printf("\nenso_ex v5.0 by skgleba | @stage1!\n\n");

    // clean after us
    memset((char*)ENSO_CORRUPTED_AREA_START, 0, ENSO_CORRUPTED_AREA_SIZE);

    // restore corrupted boot args with our copy
    memcpy(boot_args, (*sysroot_ctx_ptr)->boot_args, sizeof(*boot_args));
	
    // memblock for stage2
    printf("[E2X] stage2 a");
    void* stage2 = NULL;
    int blk = sceKernelAllocMemBlock("", MEMBLOCK_TYPE_RW, SECOND_PAYLOAD_SIZE, NULL);
    sceKernelGetMemBlockBase(blk, &stage2);
    
    // read stage2 from emmc
    printf("-r");
    read_sector_default((int*)NSKBL_DEVICE_EMMC_CTX, SECOND_PAYLOAD_OFFSET, SECOND_PAYLOAD_SIZE / SDIF_SECTOR_SIZE, (int)stage2);

    // rw->rx
    printf("-m");
    sceKernelRemapBlock(blk, MEMBLOCK_TYPE_RX);
    clean_dcache(stage2, SECOND_PAYLOAD_SIZE);
    flush_icache();

    // run stage2
    printf("-x\n");
    void (*stage2_start)() = (void*)(stage2 + 1);
    stage2_start();

    // say bye
    printf("[E2X] resume nskbl\n");

    // restore context and resume boot
    uint32_t* sp = *(uint32_t**)(ENSO_SP_AREA_OFFSET + ENSO_SP_TOP_CORE0); // sp top for core 0
    uint32_t* old_sp = sp - ENSO_SP_TOP_OLD_CORE0;

    // r0: 0x51167784 os0_dev
    // r1: 0xfffffffe
    // r2: sp - 0x110
    // r3: 0
    __asm__ volatile (
        "movw r0, #0x7784\n"
        "movt r0, #0x5116\n"
        "movw r1, #0xfffe\n"
        "movt r1, #0xffff\n"
        "mov r2, %0\n"
        "mov r3, #0\n"
        "mov sp, %1\n"
        "mov r4, %2\n"
        "bx r4\n" :: "r" (sp - 0x110), "r" (old_sp), "r" (0x5101F779) : "r0", "r1", "r2", "r3", "r4"
    );
}

__attribute__ ((section (".text.start"), naked)) void start(void)  {
    __asm__ volatile (
        "mov r0, #0\n"
        "movt r0, #0x51f0\n"
        "mov sp, r0\n"
        "b go\n"
    );
}
