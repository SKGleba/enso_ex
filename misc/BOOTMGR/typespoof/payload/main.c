/*
	lv0 type spoof for 3.60-3.70
	since 3.71 there is protection on ks x509
*/

#include "types.h"

void _start(void) {
	char buf[4];
	*(u32_t *)buf = *(u32_t *)0xE0062124; // get type from keyslot x509+4
	*(u16_t *)buf = *(u16_t *)0x1f850300; // change type
	
	// copy keyslot x509 to the KR_PROG
	*(u32_t *)0xE0030000 = *(u32_t *)0xE0062120;
	*(u32_t *)0xE0030004 = *(u32_t *)buf; // patched type
	*(u32_t *)0xE0030008 = *(u32_t *)0xE0062128;
	*(u32_t *)0xE003000C = *(u32_t *)0xE006212C;
	*(u32_t *)0xE0030010 = *(u32_t *)0xE0062130;
	*(u32_t *)0xE0030014 = *(u32_t *)0xE0062134;
	*(u32_t *)0xE0030018 = *(u32_t *)0xE0062138;
	*(u32_t *)0xE003001C = *(u32_t *)0xE006213C;
	
	// tell KR_PROG to write 0xE0030000 (0x20) to the KR in KS x509
	*(u32_t *)0xE0030020 = (u32_t)0x00000509;
	return;
}