/* 
	NMFramework by SKGleba
	This software may be modified and distributed under the terms of the MIT license.
	See the LICENSE file for details.
*/
#include "../spl-defs.h"

// Check manager magic and jump to it
int _start(void) {
	*(u16_t *)0x00804d06 = (u16_t)0xc521;
	*(u16_t *)0x00804d08 = (u16_t)0;
	*(u16_t *)0x00804d0a = (u16_t)0x5001;
	
	if (*(u8_t *)0x1f850200 > 0) {
		// sm_load::set_state(5) -> jsr (0x00807c50)
		*(u16_t *)0x00800a06 = 0xd050;
		*(u16_t *)0x00800a08 = 0x807c;
		*(u16_t *)0x00800a0a = 0x100f;
		u32_t (*pacpy)(int dst, int src, int sz) = (void*)((u32_t)0x00801016); // memcpy func
		*(u32_t *)0x1f85020c = pacpy((u32_t)0x00807c50, (u32_t)0x1f850220, 0x50);
	}
	return 0;
}
