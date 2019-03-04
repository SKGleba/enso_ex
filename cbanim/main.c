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
	
	char flags[4] = { 0, 1, 0, 0 };
	
	int compress = 1, l = 1, png = 0, gif = 0;
	FILE *fp;
	if(argc < 4){
		printf("\nusage: cbanim -intype -loop? filename%%d -opt1\n intypes:\n  -r: raw\n  -p: png/jpg/any supported ststic image format\n  -g: gif\n Note: you can add a \"s\" to resize.\n\n  ex1: cbanim -p -y frame_%%d.png => add PNGs to the bootanimation starting from frame_0.png, set it to loop mode\n  ex2: cbanim -sg -n anim.gif => resize and convert the anim.gif GIF to the bootanimation, set it to run-once mode\n\n");
		return -1;
	}
	if (argc == 5){
         flags[1] = 0;
		 compress = 0;
    }
	if (strcmp("-y", argv[2]) == 0){
         flags[0] = 1;
    }
	if (strcmp("-g", argv[1]) == 0){
         gif = 1;
		 png = 1;
    } else if (strcmp("-p", argv[1]) == 0){
         png = 1;
    } else if (strcmp("-sg", argv[1]) == 0){
         gif = 2;
		 png = 2;
    } else if (strcmp("-sp", argv[1]) == 0){
         png = 2;
    }
	
	if (gif > 0) {
		printf("extracting...\n");
		sprintf(cbuff, argv[3], 0);
		printf("wait ( %s )\n", cbuff);
		sprintf(syscmdg, "convert %s frame.png", cbuff);
		system(syscmdg);
		cur++;
		printf("...done\n");
		argv[3] = "frame-%d.png";
	}
	
	cur = 0;
	
	if (png > 0) printf("converting...\n");
	while (png > 0) {
		sprintf(cbuff, argv[3], cur);
		fp = fopen(cbuff, "rb");
		if (fp == NULL) break;
		fclose(fp);
		printf("converting %s\n", cbuff);
		if (png == 2) {
			sprintf(syscmdg, "convert %s -resize 960x544! frame_%d.rgba", cbuff, cur);
		} else {
			sprintf(syscmdg, "convert %s frame_%d.rgba", cbuff, cur);
		}
		system(syscmdg);
		if (gif > 0) unlink(cbuff);
		cur++;
	}
	if (png > 0) {
		printf("...done\n");
		png = 1;
	}
	
	cur = 0;
	
	if (compress == 1) printf("compressing...\n");
	while (compress == 1 && png == 0) {
		sprintf(cbuff, argv[3], cur);
		fp = fopen(cbuff, "rb");
		if (fp == NULL) break;
		fclose(fp);
		printf("compressing %s\n", cbuff);
		sprintf(syscmdg, "gzip -9 -k %s", cbuff);
		system(syscmdg);
		cur++;
	}
	while (compress == 1 && png == 1) {
		sprintf(cbuff, "frame_%d.rgba", cur);
		fp = fopen(cbuff, "rb");
		if (fp == NULL) break;
		fclose(fp);
		printf("compressing %s\n", cbuff);
		sprintf(syscmdg, "gzip -9 -k %s", cbuff);
		system(syscmdg);
		unlink(cbuff);
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
			if (png == 0) {
				sprintf(cbuff, argv[3], cur);
				sprintf(mbuff, "%s.gz", cbuff);
			} else {
				sprintf(mbuff, "frame_%d.rgba.gz", cur);
			}
		} else {
			if (png == 0) {
				sprintf(mbuff, argv[3], cur);
			} else {
				sprintf(mbuff, "frame_%d.rgba", cur);
			}
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
