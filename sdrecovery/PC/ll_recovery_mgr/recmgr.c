// Flash/configure SD for low-level enso_ex recovery

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include "../../../enso/ex_defs.h"

#define BLOCK_SIZE 0x200
#define RECOVERY_OFFSET 0x400
#define RECOVERY_SIZE 0x4000
#define DO_LOAD_GCSD_KERNEL 0
#define DO_USE_RECOVERY 1

uint8_t enable = DO_USE_RECOVERY, extkern = DO_LOAD_GCSD_KERNEL;
uint32_t bloff = RECOVERY_OFFSET/0x200, blsz = RECOVERY_SIZE/0x200, magic = E2X_MAGIC, foff = E2X_RECOVERY_BLKOFF;

static unsigned char recblk[0x10];

void info(const char *dev, int set) {
	FILE *fp = fopen(dev, "rb+");
	RecoveryBlockStruct *recoveryblock = (RecoveryBlockStruct *)recblk;
	if (set == 0) {
		fread(recblk,1,0x10,fp);
		magic = recoveryblock->magic;
		extkern = recoveryblock->flags[0];
		enable = recoveryblock->flags[1];
		bloff = recoveryblock->blkoff;
		blsz = recoveryblock->blksz;
	} else if (set == 1) {
		recoveryblock->magic = magic;
		recoveryblock->flags[0] = extkern;
		recoveryblock->flags[1] = enable;
		recoveryblock->flags[2] = recoveryblock->flags[3] = 0;
		recoveryblock->blkoff = bloff;
		recoveryblock->blksz = blsz;
		fwrite(recblk,1,0x10,fp);
	}
	fclose(fp);
	printf("\n magic: %X\n enabled: %s\n use SD os0: %s\n block size: %d bytes\n offset: block %d\n size: %d blocks\n", magic, (enable == 1) ? "yes" : "no", (extkern == 1) ? "yes" : "no", BLOCK_SIZE, bloff, blsz);
}

void pseudodd(const char *src, const char *dst, uint32_t iseek, uint32_t oseek, uint32_t size) {
	printf("\npseudodd: [%d]:%s[%d]->%s[%d]\n", size, src, iseek, dst, oseek);
	unsigned char * buffer = (unsigned char *) malloc (size);
	printf("reading source...\n");
	FILE *fp = fopen(src,"rb");
	fseek(fp,iseek,SEEK_SET);
	fread(buffer,1,size,fp);
	fclose(fp);
	printf("writing to target...\n");
	FILE *fl = fopen(dst, "rb+");
	if (fl == NULL) {
		fl = fopen(dst, "wb");
		fclose(fl);
		fl = fopen(dst, "rb+");
	}
	fseek(fl,oseek,SEEK_SET);
	fwrite(buffer,1,size,fl);
	fclose(fl);
	free(buffer);
	printf("done !\n");
}

void mgr(const char *src, const char *dst, int set) {
	printf("\n%s[%s]...\n", (set == 0) ? "dumping" : "writing", src);
	if (set == 0)
		info(src, 0);
	pseudodd(src, dst, (set == 0) ? bloff * 0x200 : foff * 0x200, (set == 0) ? foff * 0x200 : bloff * 0x200, blsz * 0x200);
	if (set == 1)
		info(dst, 1);
	printf("\n");
}

int main (int argc, char *argv[]) {

	if(argc < 4){
		printf ("\nusage: sudo ./[binname] [mode] [source] [target] [opt flags]\n");
		return -1;
	}
  
  // optional flags
	for (int i=3; i< argc; i++) {
     	if (strcmp("-disable", argv[i]) == 0)
       		enable = 0;
    	else if (strcmp("-extkern", argv[i]) == 0)
        	extkern = 1;
        else if (strcmp("-off", argv[i]) == 0 && i < argc) {
        	i = i + 1;
        	bloff = atoi(argv[i]);
        } else if (strcmp("-sz", argv[i]) == 0 && i < argc) {
        	i = i + 1;
        	blsz = atoi(argv[i]);
        } else if (strcmp("-foff", argv[i]) == 0 && i < argc) {
        	i = i + 1;
        	foff = atoi(argv[i]);
        }
 	}

  // modes
	if (strcmp("-r", argv[1]) == 0) {
        mgr(argv[2], argv[3], 0);
    } else if (strcmp("-w", argv[1]) == 0) {
        mgr(argv[2], argv[3], 1);
    } else if (strcmp("-dd", argv[1]) == 0) {
        pseudodd(argv[2], argv[3], foff, bloff, blsz);
    }
 
 	return 0;
}