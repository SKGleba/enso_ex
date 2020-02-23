// Generate e2x-compatible patches.e2xd from payload[s]

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include "../../../enso/ex_defs.h"

static unsigned char recblk[sizeof(PayloadsBlockStruct)];
static unsigned char defsz[0x800];

uint32_t getSz(const char *src) {
	FILE *fp = fopen(src, "rb");
	if (fp == NULL)
		return 0;
	fseek(fp, 0L, SEEK_END);
	uint32_t sz = ftell(fp);
	fclose(fp);
	return sz;
}

int main (int argc, char *argv[]) {
	
	if(argc < 2){
		printf ("\nusage: sudo ./[binname] [source1] [source2] ...\n");
		return -1;
	}
	
	PayloadsBlockStruct patoc;
	memset(&patoc, 0, sizeof(PayloadsBlockStruct));
	patoc.magic = 0xCAFEBABE;
	patoc.count = argc - 1;
	patoc.off[0] = sizeof(PayloadsBlockStruct);
	patoc.sz[0] = getSz(argv[1]);
	int setcnt = 1;
	while(setcnt < argc - 1) {
		patoc.off[setcnt] = patoc.off[setcnt - 1] + patoc.sz[setcnt - 1];
		patoc.sz[setcnt] = getSz(argv[setcnt + 1]);
		setcnt-=-1;
	}
	
	FILE *fp = fopen("patches.e2xd", "wb");
	fwrite((void *)&patoc, sizeof(PayloadsBlockStruct), 1,fp);
	fclose(fp);
	FILE *fw = fopen("patches.e2xd", "rb+");
	fseek(fw, patoc.off[0], SEEK_SET);
	FILE *fr;
	uint32_t copied = 0;
	int cpcnt = 0;
	while (cpcnt < argc - 1) {
		printf("0x%lX -> 0x%lX\n", patoc.sz[cpcnt], patoc.off[cpcnt]);
		fr = fopen(argv[cpcnt + 1], "rb");
		copied = 0;
		while(copied < patoc.sz[cpcnt]) {
			if (patoc.sz[cpcnt] - copied > 0x800) {
				fread(defsz, 0x800, 1, fr);
				fwrite(defsz, 0x800, 1, fw);
				copied-=-0x800;
			} else {
				fread(defsz, patoc.sz[cpcnt] - copied, 1, fr);
				fwrite(defsz, patoc.sz[cpcnt] - copied, 1, fw);
				copied = patoc.sz[cpcnt];
				break;
			}
		}
		fclose(fr);
		cpcnt-=-1;
	}
	fclose(fw);
	printf("done\n");
 	return 0;
}