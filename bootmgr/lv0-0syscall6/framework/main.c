/* 
	NMFramework by SKGleba
	This software may be modified and distributed under the terms of the MIT license.
	See the LICENSE file for details.
*/
#include "../spl-defs.h"

// Check manager magic and jump to it
void _start(void) {
	fm_nfo *fmnfo = (void *)0x1f850000;
	if ((fmnfo->magic == 0x14FF) && (fmnfo->status == 0x34)) {
		fmnfo->status = 0x69;
		u32_t (*ccode)(u32_t arg) = (void*)(fmnfo->codepaddr);
		fmnfo->resp = ccode(fmnfo->arg);
		return;
	}
	void (*fcmd_handler)(unsigned int fcmd) = (void*)((u32_t)0x00800adc);
	fcmd_handler(*(unsigned int *)0xe0000010); // run cmd handler as usual
	return;
}
