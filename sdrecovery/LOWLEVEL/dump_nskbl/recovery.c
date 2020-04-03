// dump NSKBL to GCSD

#include <inttypes.h>
#ifdef FW_360
	#include "../../../enso/360/nsbl.h"
#else
	#include "../../../enso/365/nsbl.h"
#endif
#include "../../../enso/ex_defs.h"
 
void __attribute__((optimize("O0"))) _start(void *kbl_param, unsigned int ctrldata) {
	
#ifdef FW_360
	// patch read_sector_sd to send a write command instead
	*(uint8_t *)0x5101e6af = 0x62;
	*(uint8_t *)0x5101e6b3 = 0x60;
	*(uint8_t *)0x5101e6ba = 0x18;
	*(uint8_t *)0x5101e6bc = 0x19;
	clean_dcache((void *)0x5101e6a0, 0x20);
    flush_icache();
	
	read_sector_sd((int *)*(uint32_t *)0x5102801c, 0, 0x51000000, 0x1000000/0x200);
	
	// undo patches
	*(uint8_t *)0x5101e6af = 0x52;
	*(uint8_t *)0x5101e6b3 = 0x50;
	*(uint8_t *)0x5101e6ba = 0x11;
	*(uint8_t *)0x5101e6bc = 0x12;
	clean_dcache((void *)0x5101e6a0, 0x20);
    flush_icache();
#else
	// patch read_sector_sd to send a write command instead
	*(uint8_t *)0x5101e8b7 = 0x62;
	*(uint8_t *)0x5101e8bb = 0x60;
	*(uint8_t *)0x5101e8c2 = 0x18;
	*(uint8_t *)0x5101e8c4 = 0x19;
	clean_dcache((void *)0x5101e8b0, 0x20);
    flush_icache();
	
	read_sector_sd((int *)*(uint32_t *)0x5102801c, 0, 0x51000000, 0x1000000/0x200);
	// undo patches
	*(uint8_t *)0x5101e8b7 = 0x52;
	*(uint8_t *)0x5101e8bb = 0x50;
	*(uint8_t *)0x5101e8c2 = 0x11;
	*(uint8_t *)0x5101e8c4 = 0x12;
	clean_dcache((void *)0x5101e8b0, 0x20);
    flush_icache();
#endif

}
