/*
    cbanim by SKGleba
    All Rights Reserved
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>



int main(int argc, char **argv){
	
	uint32_t cur = 0, maxx = 0, sz = 0;
	
	char mbuff[128], cbuff[128], syscmdc[128], syscmdg[128], max[4];
	
	char flags[4] = { 1, 0, 0, 0 };
	
	int compress = 1, l = 1;
	FILE *fp;
	if(argc < 3){
		printf("\nusage: cbanim -loop? filename%%d -opt1\n ex: cbanim -y boot_splash.bin(%%d)\n\n");
		return -1;
	}
	if (argc == 4){
         flags[0] = 0;
		 compress = 0;
    }
	if (strcmp("-y", argv[1]) == 0){
         flags[1] = 1;
    }
	if (compress == 1) printf("compressing...\n");
	while (compress == 1) {
		sprintf(cbuff, argv[2], cur);
		fp = fopen(cbuff, "rb");
		if (fp == NULL) break;
		fclose(fp);
		printf("compressing %s\n", cbuff);
		sprintf(syscmdg, "gzip -9 -k %s", cbuff);
		system(syscmdg);
		cur++;
	}
	if (compress == 1) printf("...done\n");
	printf("creating output file [boot_animation.img]\n");
	cur = 0;
	fp = fopen("boot_animation.img", "wb+");
	memcpy(max, &maxx, sizeof(maxx));
	fwrite(max, 4, 1, fp);
	fwrite(flags, 4, 1, fp);
	fclose(fp);
	printf("combining [boot_animation.img]...\n");
	while (l == 1) {
		if (compress == 1) {
			sprintf(cbuff, argv[2], cur);
			sprintf(mbuff, "%s.gz", cbuff);
		} else {
			sprintf(mbuff, argv[2], cur);
		}
		fp = fopen(mbuff, "rb");
		if (fp == NULL) {
			if (cur > 0) maxx = cur - 1;
			fp = fopen("boot_animation.img", "rb+");
			memcpy(max, &maxx, sizeof(maxx));
			fwrite(max, 4, 1, fp);
			fclose(fp);
			printf("...done [boot_animation.img]\n");
			l = 0;
			exit(0);
		}
		printf("adding %s\n", mbuff);
		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		memcpy(max, &sz, sizeof(sz));
		fclose(fp);
		fp = fopen("boot_animation.img", "ab");
		fwrite(max, 4, 1, fp);
		fclose(fp);
		sprintf(syscmdg, "cat %s >> boot_animation.img", mbuff);
		system(syscmdg);
		if (compress == 1) unlink(mbuff);
		cur++;
	}
	
	return 1;
}
