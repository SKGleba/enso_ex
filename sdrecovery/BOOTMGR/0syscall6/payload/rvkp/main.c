/* 
	0syscall6 by SKGleba
	All Rights Reserved
*/
#include "types.h"

/*
before:
        00804d06 49 da 0c 00     bsr        FUN_0080594e
        00804d0a 00 05           mov        r5,r0

after:
        00804d06 21 c5 00 00     movh       r5,0x0
        00804d0a 01 50           mov        r0,0x1
next:
        00804d0c 04 e0 9e 01     bnei       r0,0x0,LAB_00805048
*/
void _start(void) {
	*(u16_t *)0x00804d06 = (u16_t)0xc521;
	*(u16_t *)0x00804d08 = (u16_t)0;
	*(u16_t *)0x00804d0a = (u16_t)0x5001;
	
	if (*(u8_t *)0x1f850200 > 0) {
		// sm_load::set_state(5) -> jsr (0x00807c50)
		*(u16_t *)0x00800a06 = 0xd000;
		*(u16_t *)0x00800a08 = 0x809e;
		*(u16_t *)0x00800a0a = 0x100f;
		u32_t (*pacpy)(int dst, int src, int sz) = (void*)((u32_t)0x00801016); // memcpy func
		*(u32_t *)0x1f85020c = pacpy((u32_t)0x00809e00, (u32_t)0x1f850220, 0x80);
	}
	return;
}
