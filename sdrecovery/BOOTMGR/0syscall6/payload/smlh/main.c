#include "../types.h"

void _start(void) {
	
	if ((*(u16_t *)0x0080b00e == (u16_t)0xb080) && (*(u32_t *)0x1f850200 == 0xDEAFFADE)) {
		u32_t (*pacpy)(int dst, int src, int sz) = (void*)((u32_t)0x00801016); // memcpy func
		*(u32_t *)0x1f85020c = pacpy((u32_t)0x0080b000, *(u32_t *)0x1f850204, *(u32_t *)0x1f850208);
	}
	
	void (*set_status)() = (void*)((u32_t)0x00802010);
	set_status(5);
	return;
}
