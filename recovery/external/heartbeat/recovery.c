#include <inttypes.h>

#include "../../../core/ex_defs.h"

// recovery block, parsed by enso_ex
__attribute__((section(".text._my_info"))) const RecoveryBlockStruct _my_info = {
	E2X_MAGIC, // expected enso_ex magic
	0, // this recovery offset on GCSD, in sectors
	1, // this recovery size on GCSD, in sectors
	0x11 // this recovery offset inside recovery sector, in bytes, |=1 for thumb
};

// for all I/O funcs: FUNC(dev_off_blk, mem_buf, blk_count)
#define SDREAD(...) read_sector_sd((int *)*(uint32_t *)NSKBL_DEVICE_GCSD_TGT_CTX, ##__VA_ARGS__)
#define SDWRITE(...) \
do { \
	set_sd_write_mode(1); \
	SDREAD(__VA_ARGS__); \
	set_sd_write_mode(0); \
} while (0)

// nskbl funcs & offs
#define NSKBL_DEVICE_GCSD_TGT_CTX 0x5102801C
static int (*read_sector_sd)(int* part_ctx, int sector, void *buffer, int nsectors) = (void*)0x5101E879;
static void (*clean_dcache)(void* dst, int len) = (void*)0x510146DD;
static void (*flush_icache)() = (void*)0x51014691;

// patch read_sector_sd to send a write command instead
static void set_sd_write_mode(int mode) {
	if (mode) {
		*(uint8_t *)0x5101e8b7 = 0x62;
		*(uint8_t *)0x5101e8bb = 0x60;
		*(uint8_t *)0x5101e8c2 = 0x18;
		*(uint8_t *)0x5101e8c4 = 0x19;
		clean_dcache((void *)0x5101e8b0, 0x20);
		flush_icache();
	} else { // undo patches
		*(uint8_t *)0x5101e8b7 = 0x52;
		*(uint8_t *)0x5101e8bb = 0x50;
		*(uint8_t *)0x5101e8c2 = 0x11;
		*(uint8_t *)0x5101e8c4 = 0x12;
		clean_dcache((void *)0x5101e8b0, 0x20);
		flush_icache();
	}
}

__attribute__((section(".text.start"), optimize("O0"))) int start(void* kbl_param, unsigned int ctrldata) {
	SDWRITE(0, kbl_param, 1);
	return 0;
}