/* 
	NMFramework by SKGleba
	This software may be modified and distributed under the terms of the MIT license.
	See the LICENSE file for details.
*/
#include "../spl-defs.h"

// Copy the framework and hook sm_load & fcmd_handle
u32_t __attribute__((optimize("O0"))) _start(void) {
	u32_t ret = 0;
	u32_t (*pacpy)(int dst, int src, int sz) = (void*)((u32_t)0x00801016); // memcpy func
	ret = pacpy((u32_t)0x00809e00, (u32_t)0x1f850200, 0x100); // copy fcmd_handle payload
		
	// fcmd_handler() -> bsr (0x00809e00)
	*(u16_t *)0x00800372 = 0xdc79;
	*(u16_t *)0x00800374 = (u16_t)0x009a;
		
	// fcmd_handler::get_cmd -> mov r6,r1
	*(u16_t *)0x00800afe = 0x0610;
	return ret;
}
